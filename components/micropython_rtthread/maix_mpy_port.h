#ifndef MAIX_MPY_PORT_H
#define MAIX_MPY_PORT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int maix_mpy_init(void);
void maix_mpy_deinit(void);
int maix_mpy_exec(const char *virtual_path, const char *source, size_t size);
int maix_mpy_exec_file(const char *virtual_path);
const char *maix_mpy_last_error(void);
size_t maix_mpy_heap_size(void);

#ifdef __cplusplus
}
#endif

#endif
