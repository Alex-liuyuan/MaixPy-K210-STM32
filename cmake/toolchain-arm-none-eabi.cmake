set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 工具链前缀
set(TOOLCHAIN_PREFIX arm-none-eabi-)

find_program(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
find_program(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump)
find_program(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "找不到 arm-none-eabi-gcc，请安装 ARM 工具链")
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 通用编译标志
set(COMMON_FLAGS
    "-ffunction-sections -fdata-sections -fno-common"
    "-Wall -Wextra -Wno-unused-parameter"
)
string(JOIN " " COMMON_FLAGS_STR ${COMMON_FLAGS})

set(CMAKE_C_FLAGS_INIT   "${COMMON_FLAGS_STR}")
set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS_STR} -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS_INIT "-x assembler-with-cpp")

set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-Wl,--gc-sections --specs=nano.specs --specs=nosys.specs"
)

# 搜索路径限制（不使用宿主机库）
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
