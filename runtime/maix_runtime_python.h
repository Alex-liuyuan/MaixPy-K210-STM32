#ifndef MAIX_RUNTIME_PYTHON_H
#define MAIX_RUNTIME_PYTHON_H

#include "maix_runtime_app.h"

#ifdef __cplusplus
extern "C" {
#endif

void maix_runtime_python_init(maix_runtime_profile_t *profile, maix_runtime_state_t *state);
void maix_runtime_python_run_boot(maix_runtime_state_t *state);
void maix_runtime_python_print_status(void);
const char *maix_runtime_python_selected_script(void);

#ifdef __cplusplus
}
#endif

#endif
