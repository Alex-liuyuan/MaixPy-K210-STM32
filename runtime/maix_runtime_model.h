#ifndef MAIX_RUNTIME_MODEL_H
#define MAIX_RUNTIME_MODEL_H

#include <stddef.h>

#include "maix_runtime_app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char *manifest_path;
    const char *task;
    const char *path;
    const char *labels_path;
    const char *path_backend;
    const char *runtime_backend;
    const char *runtime_reason;
    const char *format;
    const char *signature;
    size_t size_bytes;
    unsigned char header[8];
    size_t header_size;
    int present;
    int readable;
} maix_runtime_model_info_t;

typedef struct
{
    const char *path;
    const char *format;
    const char *runtime_backend;
    const char *runtime_reason;
    const char *last_error;
    size_t last_input_size;
    size_t last_output_size;
    int loaded;
} maix_runtime_model_session_info_t;

void maix_runtime_model_init(maix_runtime_profile_t *profile, maix_runtime_state_t *state);
void maix_runtime_model_refresh(maix_runtime_profile_t *profile, maix_runtime_state_t *state);
int maix_runtime_model_load(const char *path);
int maix_runtime_model_load_default(void);
int maix_runtime_model_forward(const unsigned char *input,
                               size_t input_size,
                               unsigned char *output,
                               size_t output_capacity,
                               size_t *output_size);
int maix_runtime_model_unload(void);
int maix_runtime_model_is_loaded(void);
const char *maix_runtime_model_last_error(void);
void maix_runtime_model_print_status(void);
const char *maix_runtime_model_active_path(void);
const maix_runtime_model_info_t *maix_runtime_model_info(void);
const maix_runtime_model_session_info_t *maix_runtime_model_session_info(void);

#ifdef __cplusplus
}
#endif

#endif
