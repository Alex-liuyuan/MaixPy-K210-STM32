#include "maix_ai_backend.h"
#include "maix_tflm_backend.h"

#include <stdio.h>
#include <string.h>

static void maix_ai_backend_write_error(char *error_buf,
                                        unsigned long error_buf_size,
                                        const char *message)
{
    if (error_buf == NULL || error_buf_size == 0)
    {
        return;
    }

    if (message == NULL)
    {
        message = "unknown";
    }

    snprintf(error_buf, (size_t)error_buf_size, "%s", message);
}

static int maix_ai_backend_stub_load(const char *model_path,
                                     void **handle,
                                     char *error_buf,
                                     unsigned long error_buf_size)
{
    (void)model_path;
    if (handle != NULL)
    {
        *handle = NULL;
    }
    maix_ai_backend_write_error(error_buf, error_buf_size, "backend load adapter pending");
    return -1;
}

static int maix_ai_backend_stub_forward(void *handle,
                                        const unsigned char *input,
                                        unsigned long input_size,
                                        unsigned char *output,
                                        unsigned long output_capacity,
                                        unsigned long *output_size,
                                        char *error_buf,
                                        unsigned long error_buf_size)
{
    (void)handle;
    (void)input;
    (void)input_size;
    (void)output;
    (void)output_capacity;
    if (output_size != NULL)
    {
        *output_size = 0;
    }
    maix_ai_backend_write_error(error_buf, error_buf_size, "backend forward adapter pending");
    return -1;
}

static int maix_ai_backend_stub_unload(void *handle,
                                       char *error_buf,
                                       unsigned long error_buf_size)
{
    (void)handle;
    maix_ai_backend_write_error(error_buf, error_buf_size, "ok");
    return 0;
}

static const maix_ai_backend_info_t g_backend_missing = {
    "none",
    "none",
    "no-model-selected",
    0,
    maix_ai_backend_stub_load,
    maix_ai_backend_stub_forward,
    maix_ai_backend_stub_unload,
};

static const maix_ai_backend_info_t g_backend_tflm_available = {
    "tflite-micro",
    "tflite",
    "ok",
    1,
    maix_tflm_backend_load,
    maix_tflm_backend_forward,
    maix_tflm_backend_unload,
};

static const maix_ai_backend_info_t g_backend_tflm_missing = {
    "tflite-micro",
    "tflite",
    "backend-not-built",
    0,
    maix_ai_backend_stub_load,
    maix_ai_backend_stub_forward,
    maix_ai_backend_stub_unload,
};

static const maix_ai_backend_info_t g_backend_k210_stub = {
    "k210-kmodel",
    "kmodel",
    "backend-adapter-pending",
    0,
    maix_ai_backend_stub_load,
    maix_ai_backend_stub_forward,
    maix_ai_backend_stub_unload,
};

static const maix_ai_backend_info_t g_backend_unknown = {
    "unknown",
    "unknown",
    "unsupported-model-format",
    0,
    maix_ai_backend_stub_load,
    maix_ai_backend_stub_forward,
    maix_ai_backend_stub_unload,
};

const maix_ai_backend_info_t *maix_ai_backend_resolve(const char *model_format)
{
    if (model_format == 0 || strcmp(model_format, "none") == 0)
    {
        return &g_backend_missing;
    }

    if (strcmp(model_format, "tflite") == 0)
    {
#if defined(MAIX_HAS_TFLITE_MICRO) && MAIX_HAS_TFLITE_MICRO
        return &g_backend_tflm_available;
#else
        return &g_backend_tflm_missing;
#endif
    }

    if (strcmp(model_format, "kmodel") == 0)
    {
        return &g_backend_k210_stub;
    }

    return &g_backend_unknown;
}

int maix_ai_backend_load(const maix_ai_backend_info_t *backend,
                         const char *model_path,
                         void **handle,
                         char *error_buf,
                         unsigned long error_buf_size)
{
    if (backend == NULL || backend->load == NULL)
    {
        maix_ai_backend_write_error(error_buf, error_buf_size, "invalid backend");
        if (handle != NULL)
        {
            *handle = NULL;
        }
        return -1;
    }

    return backend->load(model_path, handle, error_buf, error_buf_size);
}

int maix_ai_backend_forward(const maix_ai_backend_info_t *backend,
                            void *handle,
                            const unsigned char *input,
                            unsigned long input_size,
                            unsigned char *output,
                            unsigned long output_capacity,
                            unsigned long *output_size,
                            char *error_buf,
                            unsigned long error_buf_size)
{
    if (backend == NULL || backend->forward == NULL)
    {
        if (output_size != NULL)
        {
            *output_size = 0;
        }
        maix_ai_backend_write_error(error_buf, error_buf_size, "invalid backend");
        return -1;
    }

    return backend->forward(handle,
                            input,
                            input_size,
                            output,
                            output_capacity,
                            output_size,
                            error_buf,
                            error_buf_size);
}

int maix_ai_backend_unload(const maix_ai_backend_info_t *backend,
                           void *handle,
                           char *error_buf,
                           unsigned long error_buf_size)
{
    if (backend == NULL || backend->unload == NULL)
    {
        maix_ai_backend_write_error(error_buf, error_buf_size, "invalid backend");
        return -1;
    }

    return backend->unload(handle, error_buf, error_buf_size);
}
