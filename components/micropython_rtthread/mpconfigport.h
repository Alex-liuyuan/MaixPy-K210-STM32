#ifndef MAIX_MPY_RTTHREAD_MPCONFIGPORT_H
#define MAIX_MPY_RTTHREAD_MPCONFIGPORT_H

#include <stdint.h>
#include <inttypes.h>

#define MICROPY_NLR_SETJMP                  (1)
#define MICROPY_ENABLE_COMPILER             (1)
#define MICROPY_QSTR_BYTES_IN_HASH          (1)
#define MICROPY_ALLOC_PATH_MAX              (128)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT      (32)
#define MICROPY_EMIT_X64                    (0)
#define MICROPY_EMIT_THUMB                  (0)
#define MICROPY_EMIT_INLINE_THUMB           (0)
#define MICROPY_COMP_MODULE_CONST           (0)
#define MICROPY_COMP_CONST                  (0)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN    (0)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN    (0)
#define MICROPY_MEM_STATS                   (0)
#define MICROPY_DEBUG_PRINTERS              (0)
#define MICROPY_ENABLE_GC                   (1)
#define MICROPY_STACK_CHECK                 (1)
#define MICROPY_GC_ALLOC_THRESHOLD          (0)
#define MICROPY_HELPER_REPL                 (1)
#define MICROPY_HELPER_LEXER_UNIX           (0)
#define MICROPY_REPL_EVENT_DRIVEN           (0)
#define MICROPY_ENABLE_SOURCE_LINE          (1)
#define MICROPY_ENABLE_DOC_STRING           (0)
#define MICROPY_ERROR_REPORTING             (MICROPY_ERROR_REPORTING_NORMAL)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_PY_ASYNC_AWAIT              (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY       (0)
#define MICROPY_PY_BUILTINS_MEMORYVIEW      (0)
#define MICROPY_PY_BUILTINS_ENUMERATE       (0)
#define MICROPY_PY_BUILTINS_FILTER          (0)
#define MICROPY_PY_BUILTINS_FROZENSET       (0)
#define MICROPY_PY_BUILTINS_REVERSED        (0)
#define MICROPY_PY_BUILTINS_SET             (0)
#define MICROPY_PY_BUILTINS_SLICE           (0)
#define MICROPY_PY_BUILTINS_PROPERTY        (0)
#define MICROPY_PY_BUILTINS_MIN_MAX         (0)
#define MICROPY_PY_BUILTINS_STR_OP_MODULO   (0)
#define MICROPY_PY___FILE__                 (1)
#define MICROPY_PY_GC                       (0)
#define MICROPY_PY_ARRAY                    (0)
#define MICROPY_PY_ATTRTUPLE                (0)
#define MICROPY_PY_COLLECTIONS              (0)
#define MICROPY_PY_MATH                     (0)
#define MICROPY_PY_CMATH                    (0)
#define MICROPY_PY_IO                       (0)
#define MICROPY_PY_STRUCT                   (0)
#define MICROPY_PY_SYS                      (1)
#define MICROPY_PY_SYS_EXIT                 (0)
#define MICROPY_PY_SYS_STDFILES             (0)
#define MICROPY_PY_SYS_STDIO_BUFFER         (0)
#define MICROPY_MODULE_FROZEN_MPY           (0)
#define MICROPY_CPYTHON_COMPAT              (0)
#define MICROPY_LONGINT_IMPL                (MICROPY_LONGINT_IMPL_NONE)
#define MICROPY_FLOAT_IMPL                  (MICROPY_FLOAT_IMPL_FLOAT)
#if defined(CONFIG_PLATFORM_K210)
#define MICROPY_USE_INTERNAL_PRINTF         (0)
#else
#define MICROPY_USE_INTERNAL_PRINTF         (1)
#endif

#if defined(__thumb__)
#define MICROPY_MAKE_POINTER_CALLABLE(p)    ((void *)((uintptr_t)(p) | 1u))
#else
#define MICROPY_MAKE_POINTER_CALLABLE(p)    ((void *)(p))
#endif

#define MP_SSIZE_MAX                        INTPTR_MAX
#define UINT_FMT                            "%" PRIuPTR
#define INT_FMT                             "%" PRIdPTR

typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef long mp_off_t;

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)

extern const struct _mp_obj_module_t mp_module_maix;
extern const struct _mp_obj_module_t mp_module_maix_hal;

#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_maix), MP_ROM_PTR(&mp_module_maix) }, \
    { MP_ROM_QSTR(MP_QSTR__maix_hal), MP_ROM_PTR(&mp_module_maix_hal) },

#include <alloca.h>

#ifndef MICROPY_HW_BOARD_NAME
#define MICROPY_HW_BOARD_NAME "SYSU_AIOTOS"
#endif

#ifndef MICROPY_HW_MCU_NAME
#if defined(CONFIG_PLATFORM_K210)
#define MICROPY_HW_MCU_NAME "K210"
#elif defined(CONFIG_PLATFORM_STM32F407)
#define MICROPY_HW_MCU_NAME "STM32F407"
#else
#define MICROPY_HW_MCU_NAME "GenericRTThread"
#endif
#endif

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS

#endif
