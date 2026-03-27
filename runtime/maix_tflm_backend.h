#ifndef MAIX_TFLM_BACKEND_H
#define MAIX_TFLM_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

int maix_tflm_backend_load(const char *model_path,
                           void **handle,
                           char *error_buf,
                           unsigned long error_buf_size);
int maix_tflm_backend_forward(void *handle,
                              const unsigned char *input,
                              unsigned long input_size,
                              unsigned char *output,
                              unsigned long output_capacity,
                              unsigned long *output_size,
                              char *error_buf,
                              unsigned long error_buf_size);
int maix_tflm_backend_unload(void *handle,
                             char *error_buf,
                             unsigned long error_buf_size);

#ifdef __cplusplus
}
#endif

#endif
