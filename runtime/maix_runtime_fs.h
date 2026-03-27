#ifndef MAIX_RUNTIME_FS_H
#define MAIX_RUNTIME_FS_H

#include <stddef.h>

#include "maix_runtime_app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char *path;
    const unsigned char *data;
    size_t size;
} maix_bundle_file_t;

void maix_runtime_fs_init(maix_runtime_profile_t *profile, maix_runtime_state_t *state);
const maix_bundle_file_t *maix_runtime_fs_find(const char *path);
const maix_bundle_file_t *maix_runtime_fs_manifest(size_t *count);
int maix_runtime_fs_path_exists(const char *path);
int maix_runtime_fs_is_file(const char *path);
int maix_runtime_fs_is_dir(const char *path);
int maix_runtime_fs_file_size(const char *path, size_t *size);
int maix_runtime_fs_read(const char *path, size_t offset, void *buf, size_t size, size_t *read_size);
int maix_runtime_fs_supports_posix(void);
int maix_runtime_fs_supports_write(void);
const char *maix_runtime_fs_storage_backend(void);
const char *maix_runtime_fs_path_backend(const char *path);
const char *maix_runtime_fs_model_root(void);
void maix_runtime_fs_print_status(void);
void maix_runtime_fs_print_manifest(void);
const char *maix_runtime_fs_auto_start_app_id(void);
const char *maix_runtime_fs_auto_start_script(void);

#ifdef __cplusplus
}
#endif

#endif
