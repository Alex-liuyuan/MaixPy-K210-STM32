#ifndef MAIX_RUNTIME_APP_H
#define MAIX_RUNTIME_APP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MAIX_CAP_UNAVAILABLE = 0,
    MAIX_CAP_AVAILABLE,
    MAIX_CAP_EXPERIMENTAL,
    MAIX_CAP_PLANNED,
} maix_cap_state_t;

typedef struct
{
    const char *product_name;
    const char *product_version;
    const char *board_name;
    const char *runtime_name;
    const char *console_name;
    const char *cpu_arch;
    const char *os_name;
    maix_cap_state_t led;
    maix_cap_state_t heap;
    maix_cap_state_t shell;
    maix_cap_state_t device_framework;
    maix_cap_state_t storage;
    maix_cap_state_t python_vm;
    maix_cap_state_t model_runtime;
    maix_cap_state_t camera;
    maix_cap_state_t display;
} maix_runtime_profile_t;

typedef struct
{
    uint32_t uptime_ms;
    unsigned long heartbeat_count;
    uint32_t heap_total;
    uint32_t heap_used;
    uint32_t heap_peak;
} maix_runtime_state_t;

void maix_board_app_init(void);
void maix_board_fill_profile(maix_runtime_profile_t *profile);
void maix_board_heartbeat(unsigned long heartbeat_count);

int maix_runtime_main(void);

#ifdef __cplusplus
}
#endif

#endif
