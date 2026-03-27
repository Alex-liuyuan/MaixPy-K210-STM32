set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(K210_TOOLCHAIN_BIN_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../third_party/toolchains/kendryte-toolchain/bin"
    CACHE PATH "Kendryte RISC-V toolchain bin directory")

find_program(CMAKE_C_COMPILER
    NAMES riscv64-unknown-elf-gcc
    HINTS "${K210_TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(CMAKE_CXX_COMPILER
    NAMES riscv64-unknown-elf-g++
    HINTS "${K210_TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(CMAKE_ASM_COMPILER
    NAMES riscv64-unknown-elf-gcc
    HINTS "${K210_TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(CMAKE_OBJCOPY
    NAMES riscv64-unknown-elf-objcopy
    HINTS "${K210_TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(CMAKE_OBJDUMP
    NAMES riscv64-unknown-elf-objdump
    HINTS "${K210_TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(CMAKE_SIZE
    NAMES riscv64-unknown-elf-size
    HINTS "${K210_TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(CMAKE_AR
    NAMES riscv64-unknown-elf-ar
    HINTS "${K210_TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(CMAKE_RANLIB
    NAMES riscv64-unknown-elf-ranlib
    HINTS "${K210_TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR
        "找不到 riscv64-unknown-elf-gcc。\n"
        "请先准备 third_party/toolchains/kendryte-toolchain/bin")
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(K210_COMMON_FLAGS
    -mcmodel=medany
    -mabi=lp64f
    -march=rv64imafc
    -ffunction-sections
    -fdata-sections
    -fno-common
    -fstrict-volatile-bitfields
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)
string(JOIN " " K210_COMMON_FLAGS_STR ${K210_COMMON_FLAGS})

set(CMAKE_C_FLAGS_INIT
    "${K210_COMMON_FLAGS_STR} -std=gnu11"
)
set(CMAKE_CXX_FLAGS_INIT
    "${K210_COMMON_FLAGS_STR} -std=gnu++17 -fno-exceptions -fno-rtti"
)
set(CMAKE_ASM_FLAGS_INIT
    "-x assembler-with-cpp -mcmodel=medany -mabi=lp64f -march=rv64imafc"
)
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-nostartfiles -static -Wl,--gc-sections -Wl,-EL -Wl,--no-relax"
)

execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-file-name=crtbegin.o
    OUTPUT_VARIABLE CRTBEGIN_OBJ
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-file-name=crtend.o
    OUTPUT_VARIABLE CRTEND_OBJ
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-file-name=crti.o
    OUTPUT_VARIABLE CRTI_OBJ
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-file-name=crtn.o
    OUTPUT_VARIABLE CRTN_OBJ
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> \"${CRTI_OBJ}\" \"${CRTBEGIN_OBJ}\" <OBJECTS> \"${CRTEND_OBJ}\" \"${CRTN_OBJ}\" -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> \"${CRTI_OBJ}\" \"${CRTBEGIN_OBJ}\" <OBJECTS> \"${CRTEND_OBJ}\" \"${CRTN_OBJ}\" -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
