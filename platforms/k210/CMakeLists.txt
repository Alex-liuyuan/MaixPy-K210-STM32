cmake_minimum_required(VERSION 3.20)

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
get_filename_component(PROJECT_ROOT "${PROJECT_ROOT}" ABSOLUTE)

set(CMAKE_TOOLCHAIN_FILE
    "${PROJECT_ROOT}/cmake/toolchain-kendryte-riscv64.cmake"
    CACHE FILEPATH "" FORCE)

project(SYSU_AIOTOS_k210 C CXX ASM)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(K210_RTTHREAD_STACK_SIZE 32768)
set(MAIX_RUNTIME_BUNDLE_DIR "${PROJECT_ROOT}/runtime_bundle" CACHE PATH "Runtime bundle directory")
set(MAIX_RUNTIME_BUNDLE_HEADER "${CMAKE_BINARY_DIR}/generated/maix_runtime_bundle_autogen.h")
file(GLOB_RECURSE MAIX_RUNTIME_BUNDLE_INPUTS CONFIGURE_DEPENDS "${MAIX_RUNTIME_BUNDLE_DIR}/*")

set(RTTHREAD_DIR "${PROJECT_ROOT}/rtthread-nano-master/rt-thread")
set(K210_SDK_DIR "${PROJECT_ROOT}/MaixPy-v1/components/kendryte_sdk/kendryte-standalone-sdk")
set(K210_SDK_LIB "${PROJECT_ROOT}/build/k210_sdk_probe/lib/libkendryte_rtthread.a")
set(K210_NNCASE_WRAPPER_LIB "${PROJECT_ROOT}/build/k210_sdk_probe/lib/nncase/libnncase-wrapper.a")
set(K210_SDK_LINKER_SCRIPT "${K210_SDK_DIR}/lds/kendryte.ld")

set(MAIX_PROJECT_ROOT "${PROJECT_ROOT}")
set(MAIX_MPY_CROSS_COMPILE
    "${PROJECT_ROOT}/third_party/toolchains/kendryte-toolchain/bin/riscv64-unknown-elf-")
set(MAIX_MPY_ARCH_CFLAGS "-mcmodel=medany -mabi=lp64f -march=rv64imafc")
set(MAIX_MPY_INCLUDE_DIRS
    "${PROJECT_ROOT}/runtime"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${RTTHREAD_DIR}/include"
    "${RTTHREAD_DIR}/components/finsh"
    "${RTTHREAD_DIR}/finsh"
    "${RTTHREAD_DIR}/libcpu/risc-v/common"
    "${K210_SDK_DIR}/lib/bsp/include"
    "${K210_SDK_DIR}/lib/drivers/include"
    "${K210_SDK_DIR}/lib/utils/include"
)
set(MAIX_MPY_PLATFORM_DEFINITIONS
    CONFIG_PLATFORM_RTTHREAD
    CONFIG_PLATFORM_K210
    ARCH_CPU_64BIT
    ARCH_RISCV_FPU
    ARCH_RISCV_FPU_S
)
set(MAIX_MPY_BOARD_NAME "SYSU_AIOTOS")
set(MAIX_MPY_MCU_NAME "K210")
set(MAIX_MPY_LIBRARY_NAME "libmicropython_k210.a")

if(NOT EXISTS "${K210_SDK_DIR}/lib/drivers/include/sysctl.h")
    message(FATAL_ERROR
        "缺少 Kendryte SDK 头文件目录: ${K210_SDK_DIR}\n"
        "请先执行: python3 project.py k210-probe")
endif()

if(NOT EXISTS "${K210_SDK_LIB}")
    message(FATAL_ERROR
        "缺少已构建的 Kendryte SDK 库: ${K210_SDK_LIB}\n"
        "请先执行: python3 project.py k210-probe")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kendryte_rtthread.ld
    ${CMAKE_BINARY_DIR}/kendryte_rtthread.ld
    @ONLY
)

add_custom_command(
    OUTPUT "${MAIX_RUNTIME_BUNDLE_HEADER}"
    COMMAND ${Python3_EXECUTABLE} "${PROJECT_ROOT}/tools/runtime_bundle.py"
            generate
            --bundle-dir "${MAIX_RUNTIME_BUNDLE_DIR}"
            --output "${MAIX_RUNTIME_BUNDLE_HEADER}"
    DEPENDS "${PROJECT_ROOT}/tools/runtime_bundle.py" ${MAIX_RUNTIME_BUNDLE_INPUTS}
    COMMENT "生成 SYSU_AIOTOS 运行时资源 bundle"
)
add_custom_target(maix_runtime_bundle DEPENDS "${MAIX_RUNTIME_BUNDLE_HEADER}")

set(RTTHREAD_K210_SRCS
    ${RTTHREAD_DIR}/src/clock.c
    ${RTTHREAD_DIR}/src/components.c
    ${RTTHREAD_DIR}/src/cpu.c
    ${RTTHREAD_DIR}/src/idle.c
    ${RTTHREAD_DIR}/src/ipc.c
    ${RTTHREAD_DIR}/src/irq.c
    ${RTTHREAD_DIR}/src/kservice.c
    ${RTTHREAD_DIR}/src/mem.c
    ${RTTHREAD_DIR}/src/memheap.c
    ${RTTHREAD_DIR}/src/mempool.c
    ${RTTHREAD_DIR}/src/object.c
    ${RTTHREAD_DIR}/src/scheduler.c
    ${RTTHREAD_DIR}/src/signal.c
    ${RTTHREAD_DIR}/src/slab.c
    ${RTTHREAD_DIR}/src/thread.c
    ${RTTHREAD_DIR}/src/timer.c
    ${RTTHREAD_DIR}/libcpu/risc-v/common/cpuport.c
    ${RTTHREAD_DIR}/libcpu/risc-v/common/context_gcc.S
    ${RTTHREAD_DIR}/libcpu/risc-v/k210/interrupt_gcc.S
    ${RTTHREAD_DIR}/libcpu/risc-v/k210/interrupt.c
    ${RTTHREAD_DIR}/libcpu/risc-v/k210/tick.c
    ${RTTHREAD_DIR}/components/finsh/shell.c
    ${RTTHREAD_DIR}/components/finsh/msh.c
    ${RTTHREAD_DIR}/components/finsh/msh_parse.c
    ${RTTHREAD_DIR}/components/finsh/cmd.c
)

add_library(rtthread_k210 STATIC ${RTTHREAD_K210_SRCS})
target_include_directories(rtthread_k210 PUBLIC
    ${RTTHREAD_DIR}/include
    ${RTTHREAD_DIR}/components/finsh
    ${RTTHREAD_DIR}/finsh
    ${RTTHREAD_DIR}/libcpu/risc-v/common
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${K210_SDK_DIR}/lib/bsp/include
    ${K210_SDK_DIR}/lib/drivers/include
    ${K210_SDK_DIR}/lib/utils/include
)
target_compile_definitions(rtthread_k210 PUBLIC
    ARCH_CPU_64BIT
    ARCH_RISCV_FPU
    ARCH_RISCV_FPU_S
    CONFIG_PLATFORM_RTTHREAD
    CONFIG_PLATFORM_K210
    __STACKSIZE__=${K210_RTTHREAD_STACK_SIZE}
)

include("${PROJECT_ROOT}/cmake/micropython.cmake")

add_library(tflite_micro INTERFACE)
target_compile_definitions(tflite_micro INTERFACE MAIX_HAS_TFLITE_MICRO=0)

add_library(kendryte_sdk STATIC IMPORTED GLOBAL)
set_target_properties(kendryte_sdk PROPERTIES
    IMPORTED_LOCATION "${K210_SDK_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES
        "${K210_SDK_DIR}/lib/bsp/include;${K210_SDK_DIR}/lib/drivers/include;${K210_SDK_DIR}/lib/utils/include"
)

if(EXISTS "${K210_NNCASE_WRAPPER_LIB}")
    add_library(kendryte_nncase_wrapper STATIC IMPORTED GLOBAL)
    set_target_properties(kendryte_nncase_wrapper PROPERTIES
        IMPORTED_LOCATION "${K210_NNCASE_WRAPPER_LIB}"
    )
endif()

add_executable(SYSU_AIOTOS_k210
    ${PROJECT_ROOT}/runtime/maix_ai_backend.c
    ${PROJECT_ROOT}/runtime/maix_runtime_app.c
    ${PROJECT_ROOT}/runtime/maix_runtime_fs.c
    ${PROJECT_ROOT}/runtime/maix_runtime_python.c
    ${PROJECT_ROOT}/runtime/maix_runtime_model.c
    ${RTTHREAD_DIR}/libcpu/risc-v/k210/startup_gcc.S
    ${CMAKE_CURRENT_SOURCE_DIR}/boot.c
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

target_include_directories(SYSU_AIOTOS_k210 PRIVATE
    ${PROJECT_ROOT}/runtime
    ${PROJECT_ROOT}/components/hal
    ${CMAKE_BINARY_DIR}/generated
)
target_compile_definitions(SYSU_AIOTOS_k210 PRIVATE
    ARCH_CPU_64BIT
    ARCH_RISCV_FPU
    ARCH_RISCV_FPU_S
    CONFIG_PLATFORM_RTTHREAD
    CONFIG_PLATFORM_K210
    __STACKSIZE__=${K210_RTTHREAD_STACK_SIZE}
)
target_link_libraries(SYSU_AIOTOS_k210 PRIVATE
    rtthread_k210
    kendryte_sdk
    micropython_port
    tflite_micro
)

if(TARGET kendryte_nncase_wrapper)
    target_link_libraries(SYSU_AIOTOS_k210 PRIVATE kendryte_nncase_wrapper)
endif()

add_dependencies(SYSU_AIOTOS_k210 maix_runtime_bundle)

set_target_properties(SYSU_AIOTOS_k210 PROPERTIES
    SUFFIX ".elf"
    OUTPUT_NAME "SYSU_AIOTOS_k210"
)

target_link_options(SYSU_AIOTOS_k210 PRIVATE
    -T${CMAKE_BINARY_DIR}/kendryte_rtthread.ld
    -Wl,-Map=${CMAKE_BINARY_DIR}/SYSU_AIOTOS_k210.map
    -Wl,--start-group
    -Wl,--end-group
)

target_link_libraries(SYSU_AIOTOS_k210 PRIVATE gcc m c)

add_custom_command(TARGET SYSU_AIOTOS_k210 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:SYSU_AIOTOS_k210>
            ${CMAKE_BINARY_DIR}/SYSU_AIOTOS_k210.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:SYSU_AIOTOS_k210>
            ${CMAKE_BINARY_DIR}/SYSU_AIOTOS_k210.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:SYSU_AIOTOS_k210>
    COMMENT "生成 K210 ELF/HEX/BIN，打印固件大小"
)
