#include "maix_runtime_model.h"
#include "maix_ai_backend.h"
#include "maix_runtime_fs.h"

#include <rtthread.h>

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAIX_MODEL_MANIFEST_PATH "/models/model.manifest"

typedef struct
{
    char task[24];
    char default_model[64];
    char tflite[64];
    char kmodel[64];
    char labels[64];
} maix_model_manifest_t;

static const char *g_model_candidates[] = {
    "/models/model.tflite",
    "/models/model.kmodel",
    "/models/main.tflite",
    "/models/main.kmodel",
};

static const char *g_active_model_path = RT_NULL;
static maix_runtime_model_info_t g_model_info;
static maix_runtime_model_session_info_t g_model_session;
static maix_model_manifest_t g_manifest;
static char g_model_session_path[96];
static char g_model_session_error[96] = "not-loaded";
static void *g_model_backend_handle = RT_NULL;
static int g_model_session_initialized = 0;

static void maix_runtime_model_session_reset(void)
{
    memset(&g_model_session, 0, sizeof(g_model_session));
    memset(g_model_session_path, 0, sizeof(g_model_session_path));
    snprintf(g_model_session_error, sizeof(g_model_session_error), "%s", "not-loaded");
    g_model_session.path = RT_NULL;
    g_model_session.format = "none";
    g_model_session.runtime_backend = "none";
    g_model_session.runtime_reason = "not-loaded";
    g_model_session.last_error = g_model_session_error;
    g_model_session.loaded = 0;
    g_model_backend_handle = RT_NULL;
    g_model_session_initialized = 1;
}

static void maix_runtime_model_session_set_error(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsnprintf(g_model_session_error, sizeof(g_model_session_error), fmt, args);
    va_end(args);
    g_model_session.last_error = g_model_session_error;
}

static void maix_runtime_model_session_set_path(const char *path)
{
    if (path == RT_NULL)
    {
        g_model_session_path[0] = '\0';
        g_model_session.path = RT_NULL;
        return;
    }

    snprintf(g_model_session_path, sizeof(g_model_session_path), "%s", path);
    g_model_session.path = g_model_session_path;
}

static void maix_runtime_model_trim(char *value)
{
    char *end;

    while (*value != '\0' && isspace((unsigned char)*value))
    {
        memmove(value, value + 1, strlen(value));
    }

    end = value + strlen(value);
    while (end > value && isspace((unsigned char)end[-1]))
    {
        end--;
    }
    *end = '\0';
}

static const char *maix_runtime_model_join_path(const char *relative_path)
{
    static char joined_path[96];

    if (relative_path == RT_NULL || relative_path[0] == '\0')
    {
        return RT_NULL;
    }

    if (relative_path[0] == '/')
    {
        return relative_path;
    }

    if (snprintf(joined_path, sizeof(joined_path), "/models/%s", relative_path) >= (int)sizeof(joined_path))
    {
        return RT_NULL;
    }

    return joined_path;
}

static void maix_runtime_model_manifest_defaults(maix_model_manifest_t *manifest)
{
    memset(manifest, 0, sizeof(*manifest));
    memcpy(manifest->task, "classification", sizeof("classification"));
}

static int maix_runtime_model_load_manifest(maix_model_manifest_t *manifest)
{
    unsigned char raw[384];
    size_t read_size = 0;
    char *line;
    char *saveptr = RT_NULL;

    maix_runtime_model_manifest_defaults(manifest);
    if (!maix_runtime_fs_is_file(MAIX_MODEL_MANIFEST_PATH))
    {
        return -1;
    }

    if (maix_runtime_fs_read(MAIX_MODEL_MANIFEST_PATH, 0, raw, sizeof(raw) - 1, &read_size) != 0)
    {
        return -1;
    }

    raw[read_size] = '\0';
    line = strtok_r((char *)raw, "\r\n", &saveptr);
    while (line != RT_NULL)
    {
        char *equals = strchr(line, '=');

        if (equals != RT_NULL)
        {
            *equals = '\0';
            maix_runtime_model_trim(line);
            maix_runtime_model_trim(equals + 1);

            if (strcmp(line, "task") == 0)
            {
                strncpy(manifest->task, equals + 1, sizeof(manifest->task) - 1);
            }
            else if (strcmp(line, "default") == 0)
            {
                strncpy(manifest->default_model, equals + 1, sizeof(manifest->default_model) - 1);
            }
            else if (strcmp(line, "tflite") == 0)
            {
                strncpy(manifest->tflite, equals + 1, sizeof(manifest->tflite) - 1);
            }
            else if (strcmp(line, "kmodel") == 0)
            {
                strncpy(manifest->kmodel, equals + 1, sizeof(manifest->kmodel) - 1);
            }
            else if (strcmp(line, "labels") == 0)
            {
                strncpy(manifest->labels, equals + 1, sizeof(manifest->labels) - 1);
            }
        }

        line = strtok_r(RT_NULL, "\r\n", &saveptr);
    }

    return 0;
}

static const char *maix_runtime_model_fallback_path(void)
{
    size_t i;

    for (i = 0; i < sizeof(g_model_candidates) / sizeof(g_model_candidates[0]); i++)
    {
        if (maix_runtime_fs_is_file(g_model_candidates[i]))
        {
            return g_model_candidates[i];
        }
    }

    return RT_NULL;
}

static const char *maix_runtime_model_probe_path(void)
{
    const char *candidate = RT_NULL;

    maix_runtime_model_load_manifest(&g_manifest);

#if defined(CONFIG_PLATFORM_K210)
    if (g_manifest.kmodel[0] != '\0')
    {
        candidate = maix_runtime_model_join_path(g_manifest.kmodel);
        if (candidate != RT_NULL && maix_runtime_fs_is_file(candidate))
        {
            return candidate;
        }
    }
#endif

    if (g_manifest.tflite[0] != '\0')
    {
        candidate = maix_runtime_model_join_path(g_manifest.tflite);
        if (candidate != RT_NULL && maix_runtime_fs_is_file(candidate))
        {
            return candidate;
        }
    }

    if (g_manifest.default_model[0] != '\0')
    {
        candidate = maix_runtime_model_join_path(g_manifest.default_model);
        if (candidate != RT_NULL && maix_runtime_fs_is_file(candidate))
        {
            return candidate;
        }
    }

    return maix_runtime_model_fallback_path();
}

static const char *maix_runtime_model_probe_labels(const char *model_path)
{
    static char derived_label_path[96];
    static const char *fallback_candidates[] = {
        "/models/labels.txt",
        "/models/model.txt",
        "/models/classes.txt",
    };
    size_t i;
    const char *ext;
    const char *manifest_labels;

    manifest_labels = maix_runtime_model_join_path(g_manifest.labels);
    if (manifest_labels != RT_NULL && maix_runtime_fs_is_file(manifest_labels))
    {
        return manifest_labels;
    }

    if (model_path != RT_NULL)
    {
        ext = strrchr(model_path, '.');
        if (ext != RT_NULL)
        {
            size_t prefix_len = (size_t)(ext - model_path);

            if (prefix_len + 4 < sizeof(derived_label_path))
            {
                memcpy(derived_label_path, model_path, prefix_len);
                memcpy(derived_label_path + prefix_len, ".txt", 5);
                if (maix_runtime_fs_is_file(derived_label_path))
                {
                    return derived_label_path;
                }
            }
        }
    }

    for (i = 0; i < sizeof(fallback_candidates) / sizeof(fallback_candidates[0]); i++)
    {
        if (maix_runtime_fs_is_file(fallback_candidates[i]))
        {
            return fallback_candidates[i];
        }
    }

    return RT_NULL;
}

static const char *maix_runtime_model_format(const char *path)
{
    if (path == RT_NULL)
    {
        return "none";
    }

    if (strstr(path, ".tflite") != RT_NULL)
    {
        return "tflite";
    }

    if (strstr(path, ".kmodel") != RT_NULL)
    {
        return "kmodel";
    }

    return "unknown";
}

static const char *maix_runtime_model_signature(const char *format, const unsigned char *header, size_t header_size)
{
    if (header == RT_NULL || header_size == 0)
    {
        return "unreadable";
    }

    if (strcmp(format, "tflite") == 0)
    {
        if (header_size >= 8 && memcmp(header + 4, "TFL3", 4) == 0)
        {
            return "flatbuffer:TFL3";
        }
        return "flatbuffer:unknown";
    }

    if (strcmp(format, "kmodel") == 0)
    {
        return "unverified";
    }

    return "unknown";
}

static void maix_runtime_model_session_sync_defaults(const char *path, const char *format)
{
    const maix_ai_backend_info_t *backend = maix_ai_backend_resolve(format);

    maix_runtime_model_session_set_path(path);
    g_model_session.format = format ? format : "none";
    g_model_session.runtime_backend = backend ? backend->name : "none";
    g_model_session.runtime_reason = backend ? backend->reason : "invalid-backend";
}

static void maix_runtime_model_probe_info(void)
{
    const maix_ai_backend_info_t *backend;
    size_t read_size = 0;

    memset(&g_model_info, 0, sizeof(g_model_info));

    g_active_model_path = maix_runtime_model_probe_path();
    g_model_info.manifest_path = maix_runtime_fs_is_file(MAIX_MODEL_MANIFEST_PATH) ? MAIX_MODEL_MANIFEST_PATH : RT_NULL;
    g_model_info.task = g_manifest.task[0] != '\0' ? g_manifest.task : "classification";
    g_model_info.path = g_active_model_path;
    g_model_info.labels_path = maix_runtime_model_probe_labels(g_active_model_path);
    g_model_info.present = g_active_model_path != RT_NULL ? 1 : 0;
    g_model_info.path_backend = g_active_model_path != RT_NULL
                                    ? maix_runtime_fs_path_backend(g_active_model_path)
                                    : "missing";
    g_model_info.format = maix_runtime_model_format(g_active_model_path);
    g_model_info.signature = "missing";

    backend = maix_ai_backend_resolve(g_model_info.format);
    g_model_info.runtime_backend = backend->name;
    g_model_info.runtime_reason = backend->reason;

    if (!g_model_info.present)
    {
        return;
    }

    if (maix_runtime_fs_file_size(g_active_model_path, &g_model_info.size_bytes) != 0)
    {
        g_model_info.signature = "stat-failed";
        return;
    }

    if (maix_runtime_fs_read(g_active_model_path,
                             0,
                             g_model_info.header,
                             sizeof(g_model_info.header),
                             &read_size) != 0)
    {
        g_model_info.signature = "read-failed";
        return;
    }

    g_model_info.header_size = read_size;
    g_model_info.readable = 1;
    g_model_info.signature =
        maix_runtime_model_signature(g_model_info.format, g_model_info.header, g_model_info.header_size);
}

void maix_runtime_model_refresh(maix_runtime_profile_t *profile, maix_runtime_state_t *state)
{
    const maix_ai_backend_info_t *backend;

    if (!g_model_session_initialized)
    {
        maix_runtime_model_session_reset();
    }

    maix_runtime_model_probe_info();
    backend = maix_ai_backend_resolve(g_model_info.format);
    if (!g_model_session.loaded)
    {
        maix_runtime_model_session_sync_defaults(g_active_model_path, g_model_info.format);
    }

    if (profile != RT_NULL)
    {
        profile->model_runtime = g_model_info.present ? MAIX_CAP_EXPERIMENTAL : MAIX_CAP_PLANNED;
    }

    if (state != RT_NULL)
    {
        state->model_backend = backend->name;
        state->model_slot = g_active_model_path ? g_active_model_path : maix_runtime_fs_model_root();
    }
}

void maix_runtime_model_init(maix_runtime_profile_t *profile, maix_runtime_state_t *state)
{
    maix_runtime_model_session_reset();
    maix_runtime_model_refresh(profile, state);
}

int maix_runtime_model_load(const char *path)
{
    const char *target_path = path;
    const char *format;
    const maix_ai_backend_info_t *backend;
    char backend_error[96];

    if (!g_model_session_initialized)
    {
        maix_runtime_model_session_reset();
    }

    if (target_path == RT_NULL || target_path[0] == '\0')
    {
        target_path = g_active_model_path != RT_NULL ? g_active_model_path : maix_runtime_model_probe_path();
    }

    if (g_model_session.loaded)
    {
        maix_runtime_model_unload();
    }

    if (target_path == RT_NULL || target_path[0] == '\0')
    {
        maix_runtime_model_session_sync_defaults(RT_NULL, "none");
        maix_runtime_model_session_set_error("no model selected");
        return -1;
    }

    if (!maix_runtime_fs_is_file(target_path))
    {
        format = maix_runtime_model_format(target_path);
        maix_runtime_model_session_sync_defaults(target_path, format);
        maix_runtime_model_session_set_error("model file missing: %s", target_path);
        return -1;
    }

    format = maix_runtime_model_format(target_path);
    backend = maix_ai_backend_resolve(format);
    maix_runtime_model_session_sync_defaults(target_path, format);
    g_model_session.loaded = 0;
    g_model_session.last_input_size = 0;
    g_model_session.last_output_size = 0;
    g_model_backend_handle = RT_NULL;

    if (maix_ai_backend_load(backend,
                             target_path,
                             &g_model_backend_handle,
                             backend_error,
                             sizeof(backend_error)) != 0)
    {
        maix_runtime_model_session_set_error("%s", backend_error);
        return -1;
    }

    g_model_session.loaded = 1;
    maix_runtime_model_session_set_error("ok");
    return 0;
}

int maix_runtime_model_load_default(void)
{
    return maix_runtime_model_load(RT_NULL);
}

int maix_runtime_model_forward(const unsigned char *input,
                               size_t input_size,
                               unsigned char *output,
                               size_t output_capacity,
                               size_t *output_size)
{
    unsigned long backend_output_size = 0;
    char backend_error[96];

    if (output_size != RT_NULL)
    {
        *output_size = 0;
    }

    if (!g_model_session.loaded)
    {
        maix_runtime_model_session_set_error("model is not loaded");
        return -1;
    }

    if (maix_ai_backend_forward(maix_ai_backend_resolve(g_model_session.format),
                                g_model_backend_handle,
                                input,
                                (unsigned long)input_size,
                                output,
                                (unsigned long)output_capacity,
                                &backend_output_size,
                                backend_error,
                                sizeof(backend_error)) != 0)
    {
        maix_runtime_model_session_set_error("%s", backend_error);
        g_model_session.last_input_size = input_size;
        g_model_session.last_output_size = 0;
        return -1;
    }

    g_model_session.last_input_size = input_size;
    g_model_session.last_output_size = (size_t)backend_output_size;
    if (output_size != RT_NULL)
    {
        *output_size = (size_t)backend_output_size;
    }
    maix_runtime_model_session_set_error("ok");
    return 0;
}

int maix_runtime_model_unload(void)
{
    char backend_error[96];

    if (!g_model_session.loaded)
    {
        maix_runtime_model_session_set_error("not-loaded");
        return 0;
    }

    if (maix_ai_backend_unload(maix_ai_backend_resolve(g_model_session.format),
                               g_model_backend_handle,
                               backend_error,
                               sizeof(backend_error)) != 0)
    {
        maix_runtime_model_session_set_error("%s", backend_error);
        return -1;
    }

    g_model_session.loaded = 0;
    g_model_backend_handle = RT_NULL;
    g_model_session.last_input_size = 0;
    g_model_session.last_output_size = 0;
    maix_runtime_model_session_set_error("ok");
    return 0;
}

int maix_runtime_model_is_loaded(void)
{
    return g_model_session.loaded;
}

const char *maix_runtime_model_last_error(void)
{
    return g_model_session.last_error ? g_model_session.last_error : "unknown";
}

static void maix_runtime_model_print_header(const maix_runtime_model_info_t *info)
{
    size_t i;

    if (info == RT_NULL || !info->readable || info->header_size == 0)
    {
        rt_kprintf("[MODEL] header=none\n");
        return;
    }

    rt_kprintf("[MODEL] header=");
    for (i = 0; i < info->header_size; i++)
    {
        rt_kprintf("%02x", info->header[i]);
        if (i + 1 < info->header_size)
        {
            rt_kprintf(" ");
        }
    }
    rt_kprintf("\n");
}

void maix_runtime_model_print_status(void)
{
    const maix_runtime_model_info_t *info = maix_runtime_model_info();
    const maix_runtime_model_session_info_t *session = maix_runtime_model_session_info();

    if (!info->present)
    {
        rt_kprintf("[MODEL] backend=none model=none manifest=%s root=%s status=planned reason=no-model-file\n",
                   info->manifest_path ? info->manifest_path : "none",
                   maix_runtime_fs_model_root());
        rt_kprintf("[MODEL] session=idle loaded=%d target=%s last_error=%s\n",
                   session->loaded,
                   session->path ? session->path : "none",
                   session->last_error ? session->last_error : "unknown");
        return;
    }

    rt_kprintf("[MODEL] backend=%s task=%s model=%s labels=%s manifest=%s source=%s format=%s bytes=%lu signature=%s status=%s\n",
               info->runtime_backend ? info->runtime_backend : "none",
               info->task ? info->task : "unknown",
               info->path,
               info->labels_path ? info->labels_path : "none",
               info->manifest_path ? info->manifest_path : "none",
               info->path_backend,
               info->format,
               (unsigned long)info->size_bytes,
               info->signature,
               info->readable ? "loadable" : "error");
    maix_runtime_model_print_header(info);
    rt_kprintf("[MODEL] runtime_reason=%s\n", info->runtime_reason ? info->runtime_reason : "unknown");
    rt_kprintf("[MODEL] session=%s target=%s input=%lu output=%lu last_error=%s\n",
               session->loaded ? "loaded" : "idle",
               session->path ? session->path : info->path,
               (unsigned long)session->last_input_size,
               (unsigned long)session->last_output_size,
               session->last_error ? session->last_error : "unknown");
}

const char *maix_runtime_model_active_path(void)
{
    return g_active_model_path;
}

const maix_runtime_model_info_t *maix_runtime_model_info(void)
{
    return &g_model_info;
}

const maix_runtime_model_session_info_t *maix_runtime_model_session_info(void)
{
    return &g_model_session;
}
