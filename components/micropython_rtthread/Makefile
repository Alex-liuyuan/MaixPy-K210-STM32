MICROPY_CORE_DIR ?= ../../MaixPy-v1/components/micropython/core
PROJECT_ROOT ?= ../..
BUILD ?= build
MAIX_MPY_CROSS_COMPILE ?= arm-none-eabi-
MAIX_MPY_ARCH_CFLAGS ?=
MAIX_MPY_INCLUDE_FLAGS ?=
MAIX_MPY_PLATFORM_CFLAGS ?=
MAIX_MPY_IDENTITY_CFLAGS ?=

include $(MICROPY_CORE_DIR)/py/mkenv.mk

QSTR_DEFS = qstrdefsport.h

include $(TOP)/py/py.mk

CROSS_COMPILE ?= $(MAIX_MPY_CROSS_COMPILE)

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(PROJECT_ROOT)/runtime
INC += $(MAIX_MPY_INCLUDE_FLAGS)

CFLAGS = $(INC) -std=gnu11 -Os -DNDEBUG $(MAIX_MPY_ARCH_CFLAGS)
CFLAGS += -ffunction-sections -fdata-sections -fno-common
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
CFLAGS += $(MAIX_MPY_PLATFORM_CFLAGS)
CFLAGS += $(MAIX_MPY_IDENTITY_CFLAGS)

SRC_C = \
	maix_mpy_port.c \
	modmaix.c \
	modmaix_hal.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c

OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
SRC_QSTR += $(SRC_C)

include $(TOP)/py/mkrules.mk
