#ifndef MAIX_AI_BACKEND_H
#define MAIX_AI_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*maix_ai_backend_load_fn)(const char *model_path,
                                       void **handle,
                                       char *error_buf,
                                       unsigned long error_buf_size);
typedef int (*maix_ai_backend_forward_fn)(void *handle,
                                          const unsigned char *input,
                                          unsigned long input_size,
                                          unsigned char *output,
                                          unsigned long output_capacity,
                                          unsigned long *output_size,
                                          char *error_buf,
                                          unsigned long error_buf_size);
typedef int (*maix_ai_backend_unload_fn)(void *handle,
                                         char *error_buf,
                                         unsigned long error_buf_size);

typedef struct
{
    const char *name;
    const char *format;
    const char *reason;
    int available;
    maix_ai_backend_load_fn load;
    maix_ai_backend_forward_fn forward;
    maix_ai_backend_unload_fn unload;
} maix_ai_backend_info_t;

const maix_ai_backend_info_t *maix_ai_backend_resolve(const char *model_format);
int maix_ai_backend_load(const maix_ai_backend_info_t *backend,
                         const char *model_path,
                         void **handle,
                         char *error_buf,
                         unsigned long error_buf_size);
int maix_ai_backend_forward(const maix_ai_backend_info_t *backend,
                            void *handle,
                            const unsigned char *input,
                            unsigned long input_size,
                            unsigned char *output,
                            unsigned long output_capacity,
                            unsigned long *output_size,
                            char *error_buf,
                            unsigned long error_buf_size);
int maix_ai_backend_unload(const maix_ai_backend_info_t *backend,
                           void *handle,
                           char *error_buf,
                           unsigned long error_buf_size);

#ifdef __cplusplus
}
#endif

#endif
