#ifndef MAIX_MPY_RTTHREAD_MPHALPORT_H
#define MAIX_MPY_RTTHREAD_MPHALPORT_H

#include <stddef.h>

#include "py/mpconfig.h"

mp_uint_t mp_hal_ticks_ms(void);
void mp_hal_set_interrupt_char(char c);
int mp_hal_stdin_rx_chr(void);
void mp_hal_stdout_tx_strn(const char *str, size_t len);
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len);

#endif
