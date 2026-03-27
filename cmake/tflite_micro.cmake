# TFLite Micro 集成
# 假设 tflite-micro 源码位于 third_party/tflite-micro/
# 使用 FetchContent 或手动指定路径

set(TFLITE_MICRO_DIR "${CMAKE_SOURCE_DIR}/third_party/tflite-micro"
    CACHE PATH "TFLite Micro 源码路径")

option(TFLITE_ENABLE_CMSIS_NN "使用 CMSIS-NN 加速" ON)

if(NOT EXISTS "${TFLITE_MICRO_DIR}/tensorflow/lite/micro/micro_interpreter.h")
    message(WARNING
        "未找到 TFLite Micro 源码 (${TFLITE_MICRO_DIR})。\n"
        "请执行: git submodule add https://github.com/tensorflow/tflite-micro.git third_party/tflite-micro\n"
        "或设置 -DTFLITE_MICRO_DIR=<路径>")
    # 创建空占位目标，避免后续 target_link_libraries 报错
    add_library(tflite_micro INTERFACE)
    target_compile_definitions(tflite_micro INTERFACE MAIX_HAS_TFLITE_MICRO=0)
    return()
endif()

# 收集 TFLite Micro 核心源文件（手动列举，不依赖其 Makefile）
file(GLOB_RECURSE TFLITE_SRCS
    "${TFLITE_MICRO_DIR}/tensorflow/lite/micro/*.cc"
    "${TFLITE_MICRO_DIR}/tensorflow/lite/micro/kernels/*.cc"
    "${TFLITE_MICRO_DIR}/tensorflow/lite/micro/memory_planner/*.cc"
    "${TFLITE_MICRO_DIR}/tensorflow/lite/kernels/internal/*.cc"
    "${TFLITE_MICRO_DIR}/tensorflow/lite/kernels/*.cc"
    "${TFLITE_MICRO_DIR}/tensorflow/lite/core/api/*.cc"
    "${TFLITE_MICRO_DIR}/tensorflow/lite/schema/*.cc"
)

# 排除测试文件
list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*_test\\.cc$")
list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*/testing/.*")

add_library(tflite_micro STATIC ${TFLITE_SRCS})

target_include_directories(tflite_micro PUBLIC
    ${TFLITE_MICRO_DIR}
    ${TFLITE_MICRO_DIR}/tensorflow/lite/micro
    ${TFLITE_MICRO_DIR}/tensorflow/lite
)

target_compile_definitions(tflite_micro PUBLIC
    MAIX_HAS_TFLITE_MICRO=1
    TF_LITE_STATIC_MEMORY
    TF_LITE_DISABLE_X86_NEON
)

target_compile_options(tflite_micro PRIVATE
    -Wno-sign-compare
    -Wno-unused-variable
    -Wno-missing-field-initializers
)

if(TFLITE_ENABLE_CMSIS_NN)
    set(CMSIS_NN_DIR "${CMAKE_SOURCE_DIR}/third_party/CMSIS_5/CMSIS/NN"
        CACHE PATH "CMSIS-NN 路径")
    if(EXISTS "${CMSIS_NN_DIR}/Source")
        file(GLOB_RECURSE CMSIS_NN_SRCS "${CMSIS_NN_DIR}/Source/*.c")
        target_sources(tflite_micro PRIVATE ${CMSIS_NN_SRCS})
        target_include_directories(tflite_micro PUBLIC
            "${CMSIS_NN_DIR}/Include"
            "${CMAKE_SOURCE_DIR}/third_party/CMSIS_5/CMSIS/Core/Include"
        )
        target_compile_definitions(tflite_micro PUBLIC CMSIS_NN)
    endif()
endif()
