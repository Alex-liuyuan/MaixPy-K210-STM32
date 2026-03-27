if(DEFINED MAIX_PROJECT_ROOT)
    set(MAIX_MPY_ROOT_DIR "${MAIX_PROJECT_ROOT}")
else()
    set(MAIX_MPY_ROOT_DIR "${CMAKE_SOURCE_DIR}")
endif()

set(MICROPYTHON_DIR "${MAIX_MPY_ROOT_DIR}/third_party/micropython"
    CACHE PATH "MicroPython 源码路径")
set(MICROPYTHON_LEGACY_DIR "${MAIX_MPY_ROOT_DIR}/MaixPy-v1/components/micropython/core")
set(MICROPYTHON_PORT_DIR "${MAIX_MPY_ROOT_DIR}/components/micropython_rtthread")

function(maix_join_prefixed_flags out_var prefix)
    set(result "")
    foreach(item IN LISTS ARGN)
        if(NOT item STREQUAL "")
            list(APPEND result "${prefix}${item}")
        endif()
    endforeach()
    string(JOIN " " joined ${result})
    set(${out_var} "${joined}" PARENT_SCOPE)
endfunction()

if(NOT EXISTS "${MICROPYTHON_DIR}/py/runtime.h" AND EXISTS "${MICROPYTHON_LEGACY_DIR}/py/runtime.h")
    set(MICROPYTHON_DIR "${MICROPYTHON_LEGACY_DIR}" CACHE PATH "MicroPython 源码路径" FORCE)
endif()

if(NOT EXISTS "${MICROPYTHON_DIR}/py/runtime.h")
    message(WARNING
        "未找到可用的 MicroPython 核心源码。\n"
        "已检查: ${MICROPYTHON_DIR}\n"
        "可选来源:\n"
        "  1. third_party/micropython\n"
        "  2. MaixPy-v1/components/micropython/core")
    add_library(micropython_port INTERFACE)
    target_compile_definitions(micropython_port INTERFACE MAIX_HAS_MICROPYTHON=0)
    return()
endif()

if(NOT EXISTS "${MICROPYTHON_PORT_DIR}/Makefile")
    message(FATAL_ERROR "缺少 MicroPython RT-Thread 端口目录: ${MICROPYTHON_PORT_DIR}")
endif()

find_program(MAIX_HOST_MAKE NAMES make gmake REQUIRED)

set(MICROPYTHON_BUILD_DIR "${CMAKE_BINARY_DIR}/micropython")
if(NOT DEFINED MAIX_MPY_LIBRARY_NAME)
    if(DEFINED PLATFORM AND NOT PLATFORM STREQUAL "")
        set(MAIX_MPY_LIBRARY_NAME "libmicropython_${PLATFORM}.a")
    else()
        set(MAIX_MPY_LIBRARY_NAME "libmicropython.a")
    endif()
endif()
set(MICROPYTHON_PORT_LIBRARY "${MICROPYTHON_PORT_DIR}/${MAIX_MPY_LIBRARY_NAME}")

if(NOT DEFINED MAIX_MPY_CROSS_COMPILE)
    if(DEFINED TOOLCHAIN_PREFIX)
        set(MAIX_MPY_CROSS_COMPILE "${TOOLCHAIN_PREFIX}")
    else()
        set(MAIX_MPY_CROSS_COMPILE "")
    endif()
endif()

if(NOT DEFINED MAIX_MPY_ARCH_CFLAGS)
    if(DEFINED CPU_FLAGS)
        set(MAIX_MPY_ARCH_CFLAGS "${CPU_FLAGS}")
    else()
        set(MAIX_MPY_ARCH_CFLAGS "")
    endif()
endif()

if(NOT DEFINED MAIX_MPY_INCLUDE_DIRS)
    set(MAIX_MPY_INCLUDE_DIRS
        "${MAIX_MPY_ROOT_DIR}/runtime"
    )

    if(DEFINED RTTHREAD_BSP_DIR AND EXISTS "${RTTHREAD_BSP_DIR}")
        list(APPEND MAIX_MPY_INCLUDE_DIRS
            "${RTTHREAD_BSP_DIR}/Core/Inc"
            "${RTTHREAD_BSP_DIR}/RT-Thread"
            "${RTTHREAD_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc"
            "${RTTHREAD_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy"
            "${RTTHREAD_BSP_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include"
            "${RTTHREAD_BSP_DIR}/Drivers/CMSIS/Core/Include"
            "${RTTHREAD_BSP_DIR}/Drivers/CMSIS/Include"
            "${RTTHREAD_BSP_DIR}/Middlewares/Third_Party/RealThread_RTOS/include"
            "${RTTHREAD_BSP_DIR}/Middlewares/Third_Party/RealThread_RTOS/components/finsh"
            "${RTTHREAD_BSP_DIR}/Middlewares/Third_Party/RealThread_RTOS/finsh"
        )
    endif()
endif()

if(NOT DEFINED MAIX_MPY_PLATFORM_DEFINITIONS)
    set(MAIX_MPY_PLATFORM_DEFINITIONS CONFIG_PLATFORM_RTTHREAD)

    if(PLATFORM STREQUAL "rtthread" OR PLATFORM STREQUAL "stm32f407")
        list(APPEND MAIX_MPY_PLATFORM_DEFINITIONS
            CONFIG_PLATFORM_STM32F407
            STM32F407xx
            USE_HAL_DRIVER
            HSE_VALUE=8000000U
        )
    elseif(PLATFORM STREQUAL "k210")
        list(APPEND MAIX_MPY_PLATFORM_DEFINITIONS CONFIG_PLATFORM_K210)
    endif()
endif()

if(NOT DEFINED MAIX_MPY_BOARD_NAME)
    set(MAIX_MPY_BOARD_NAME "SYSU_AIOTOS")
endif()

if(NOT DEFINED MAIX_MPY_MCU_NAME)
    if(PLATFORM STREQUAL "k210")
        set(MAIX_MPY_MCU_NAME "K210")
    elseif(PLATFORM STREQUAL "rtthread" OR PLATFORM STREQUAL "stm32f407")
        set(MAIX_MPY_MCU_NAME "STM32F407")
    else()
        set(MAIX_MPY_MCU_NAME "GenericRTThread")
    endif()
endif()

maix_join_prefixed_flags(MAIX_MPY_INCLUDE_FLAGS "-I" ${MAIX_MPY_INCLUDE_DIRS})
maix_join_prefixed_flags(MAIX_MPY_PLATFORM_CFLAGS "-D" ${MAIX_MPY_PLATFORM_DEFINITIONS})

string(REPLACE "\"" "\\\"" MAIX_MPY_BOARD_NAME_ESCAPED "${MAIX_MPY_BOARD_NAME}")
string(REPLACE "\"" "\\\"" MAIX_MPY_MCU_NAME_ESCAPED "${MAIX_MPY_MCU_NAME}")
set(MAIX_MPY_IDENTITY_CFLAGS
    "-DMICROPY_HW_BOARD_NAME=\\\"${MAIX_MPY_BOARD_NAME_ESCAPED}\\\" -DMICROPY_HW_MCU_NAME=\\\"${MAIX_MPY_MCU_NAME_ESCAPED}\\\"")

add_custom_command(
    OUTPUT "${MICROPYTHON_PORT_LIBRARY}"
    COMMAND "${MAIX_HOST_MAKE}" -C "${MICROPYTHON_PORT_DIR}"
            "BUILD=${MICROPYTHON_BUILD_DIR}"
            "PROJECT_ROOT=${MAIX_MPY_ROOT_DIR}"
            "MICROPY_CORE_DIR=${MICROPYTHON_DIR}"
            "MAIX_MPY_CROSS_COMPILE=${MAIX_MPY_CROSS_COMPILE}"
            "MAIX_MPY_ARCH_CFLAGS=${MAIX_MPY_ARCH_CFLAGS}"
            "MAIX_MPY_INCLUDE_FLAGS=${MAIX_MPY_INCLUDE_FLAGS}"
            "MAIX_MPY_PLATFORM_CFLAGS=${MAIX_MPY_PLATFORM_CFLAGS}"
            "MAIX_MPY_IDENTITY_CFLAGS=${MAIX_MPY_IDENTITY_CFLAGS}"
            "LIBMICROPYTHON=${MICROPYTHON_PORT_LIBRARY}"
            clean lib
    DEPENDS
        "${MICROPYTHON_PORT_DIR}/Makefile"
        "${MICROPYTHON_PORT_DIR}/mpconfigport.h"
        "${MICROPYTHON_PORT_DIR}/mphalport.h"
        "${MICROPYTHON_PORT_DIR}/qstrdefsport.h"
        "${MICROPYTHON_PORT_DIR}/maix_mpy_port.h"
        "${MICROPYTHON_PORT_DIR}/maix_mpy_port.c"
        "${MICROPYTHON_PORT_DIR}/modmaix.c"
        "${MICROPYTHON_DIR}/py/py.mk"
        "${MICROPYTHON_DIR}/py/mkrules.mk"
    WORKING_DIRECTORY "${MAIX_MPY_ROOT_DIR}"
    COMMENT "构建 MicroPython RT-Thread 最小端口"
    VERBATIM
)

add_custom_target(micropython_port_build DEPENDS "${MICROPYTHON_PORT_LIBRARY}")

add_library(micropython_port INTERFACE)
add_dependencies(micropython_port micropython_port_build)
target_include_directories(micropython_port INTERFACE "${MICROPYTHON_PORT_DIR}")
target_link_libraries(micropython_port INTERFACE "${MICROPYTHON_PORT_LIBRARY}")
target_compile_definitions(micropython_port INTERFACE MAIX_HAS_MICROPYTHON=1)

message(STATUS "MicroPython source: ${MICROPYTHON_DIR}")
message(STATUS "MicroPython port: ${MICROPYTHON_PORT_DIR}")
message(STATUS "MicroPython cross prefix: ${MAIX_MPY_CROSS_COMPILE}")
message(STATUS "MicroPython MCU name: ${MAIX_MPY_MCU_NAME}")
