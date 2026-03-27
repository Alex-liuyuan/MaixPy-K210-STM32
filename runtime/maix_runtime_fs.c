#include "maix_runtime_fs.h"

#include <rtthread.h>

#include <fcntl.h>
#include <string.h>
#ifdef RT_USING_DFS
#include <sys/stat.h>
#include <unistd.h>
#endif

#define MAIX_RUNTIME_MODEL_ROOT "/models"

static const char *g_storage_backend = "rom-bundle";

#include "maix_runtime_bundle_autogen.h"

void maix_runtime_fs_init(maix_runtime_profile_t *profile, maix_runtime_state_t *state)
{
    g_storage_backend = maix_runtime_fs_supports_posix() ? "rom-bundle+dfs" : "rom-bundle";

    if (profile != RT_NULL)
    {
        profile->storage = maix_runtime_fs_supports_write() ? MAIX_CAP_AVAILABLE : MAIX_CAP_EXPERIMENTAL;
    }

    if (state != RT_NULL)
    {
        state->storage_backend = g_storage_backend;
        state->active_script = maix_runtime_fs_auto_start_script();
    }
}

const maix_bundle_file_t *maix_runtime_fs_find(const char *path)
{
    size_t i;

    if (path == RT_NULL)
    {
        return RT_NULL;
    }

    for (i = 0; i < MAIX_RUNTIME_BUNDLE_FILE_COUNT; i++)
    {
        if (strcmp(g_bundle_files[i].path, path) == 0)
        {
            return &g_bundle_files[i];
        }
    }

    return RT_NULL;
}

const maix_bundle_file_t *maix_runtime_fs_manifest(size_t *count)
{
    if (count != RT_NULL)
    {
        *count = MAIX_RUNTIME_BUNDLE_FILE_COUNT;
    }

    return g_bundle_files;
}

int maix_runtime_fs_supports_posix(void)
{
#ifdef RT_USING_DFS
    return 1;
#else
    return 0;
#endif
}

int maix_runtime_fs_supports_write(void)
{
    return maix_runtime_fs_supports_posix();
}

const char *maix_runtime_fs_storage_backend(void)
{
    return g_storage_backend;
}

const char *maix_runtime_fs_model_root(void)
{
    return MAIX_RUNTIME_MODEL_ROOT;
}

int maix_runtime_fs_path_exists(const char *path)
{
    if (maix_runtime_fs_find(path) != RT_NULL)
    {
        return 1;
    }

#ifdef RT_USING_DFS
    if (path != RT_NULL)
    {
        struct stat st;

        if (stat(path, &st) == 0)
        {
            return 1;
        }
    }
#endif

    return 0;
}

int maix_runtime_fs_is_file(const char *path)
{
    if (maix_runtime_fs_find(path) != RT_NULL)
    {
        return 1;
    }

#ifdef RT_USING_DFS
    if (path != RT_NULL)
    {
        struct stat st;

        if (stat(path, &st) == 0)
        {
#ifdef S_ISREG
            return S_ISREG(st.st_mode) ? 1 : 0;
#else
            return ((st.st_mode & S_IFMT) == S_IFREG) ? 1 : 0;
#endif
        }
    }
#endif

    return 0;
}

int maix_runtime_fs_is_dir(const char *path)
{
    size_t count;
    size_t i;
    size_t path_len;
    const maix_bundle_file_t *files;

    if (path == RT_NULL)
    {
        return 0;
    }

    files = maix_runtime_fs_manifest(&count);
    path_len = strlen(path);
    for (i = 0; i < count; i++)
    {
        if (strncmp(files[i].path, path, path_len) == 0
            && files[i].path[path_len] == '/'
            && path_len > 0)
        {
            return 1;
        }
    }

#ifdef RT_USING_DFS
    {
        struct stat st;

        if (stat(path, &st) == 0)
        {
#ifdef S_ISDIR
            return S_ISDIR(st.st_mode) ? 1 : 0;
#else
            return ((st.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
#endif
        }
    }
#endif

    return 0;
}

int maix_runtime_fs_file_size(const char *path, size_t *size)
{
    const maix_bundle_file_t *file = maix_runtime_fs_find(path);

    if (size != RT_NULL)
    {
        *size = 0;
    }

    if (file != RT_NULL)
    {
        if (size != RT_NULL)
        {
            *size = file->size;
        }
        return 0;
    }

#ifdef RT_USING_DFS
    if (path != RT_NULL)
    {
        struct stat st;

        if (stat(path, &st) == 0)
        {
            if (size != RT_NULL)
            {
                *size = (size_t)st.st_size;
            }
            return 0;
        }
    }
#endif

    return -1;
}

int maix_runtime_fs_read(const char *path, size_t offset, void *buf, size_t size, size_t *read_size)
{
    const maix_bundle_file_t *file = maix_runtime_fs_find(path);

    if (read_size != RT_NULL)
    {
        *read_size = 0;
    }

    if (buf == RT_NULL && size != 0)
    {
        return -1;
    }

    if (file != RT_NULL)
    {
        size_t remaining;
        size_t copy_size;

        if (offset >= file->size)
        {
            return 0;
        }

        remaining = file->size - offset;
        copy_size = size < remaining ? size : remaining;
        if (copy_size != 0)
        {
            memcpy(buf, file->data + offset, copy_size);
        }
        if (read_size != RT_NULL)
        {
            *read_size = copy_size;
        }
        return 0;
    }

#ifdef RT_USING_DFS
    if (path != RT_NULL)
    {
        int fd = open(path, O_RDONLY, 0);

        if (fd < 0)
        {
            return -1;
        }

        if (lseek(fd, (off_t)offset, SEEK_SET) < 0)
        {
            close(fd);
            return -1;
        }

        if (size != 0)
        {
            int res = read(fd, buf, size);

            if (res < 0)
            {
                close(fd);
                return -1;
            }
            if (read_size != RT_NULL)
            {
                *read_size = (size_t)res;
            }
        }

        close(fd);
        return 0;
    }
#endif

    return -1;
}

const char *maix_runtime_fs_path_backend(const char *path)
{
    if (maix_runtime_fs_find(path) != RT_NULL)
    {
        return "bundle";
    }

#ifdef RT_USING_DFS
    if (path != RT_NULL)
    {
        struct stat st;

        if (stat(path, &st) == 0)
        {
            return "dfs";
        }
    }
#endif

    return "missing";
}

void maix_runtime_fs_print_status(void)
{
    rt_kprintf("[FS] backend=%s writable=%s posix=%s model_root=%s external_models=%s app=%s\n",
               maix_runtime_fs_storage_backend(),
               maix_runtime_fs_supports_write() ? "yes" : "no",
               maix_runtime_fs_supports_posix() ? "yes" : "no",
               maix_runtime_fs_model_root(),
               maix_runtime_fs_is_dir(maix_runtime_fs_model_root()) ? "present" : "missing",
               maix_runtime_fs_auto_start_app_id());
}

void maix_runtime_fs_print_manifest(void)
{
    size_t count;
    size_t i;
    const maix_bundle_file_t *files = maix_runtime_fs_manifest(&count);

    maix_runtime_fs_print_status();
    rt_kprintf("[FS] bundle_files=%lu\n", (unsigned long)count);
    for (i = 0; i < count; i++)
    {
        rt_kprintf("[FS] %s (%lu bytes)\n",
                   files[i].path,
                   (unsigned long)files[i].size);
    }
}

const char *maix_runtime_fs_auto_start_app_id(void)
{
    return MAIX_RUNTIME_BUNDLE_AUTO_START_APP_ID;
}

const char *maix_runtime_fs_auto_start_script(void)
{
    return MAIX_RUNTIME_BUNDLE_AUTO_START_SCRIPT;
}
