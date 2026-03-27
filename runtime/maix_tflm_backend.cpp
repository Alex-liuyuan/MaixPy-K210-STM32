#include "maix_tflm_backend.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "maix_runtime_fs.h"

#if defined(MAIX_HAS_TFLITE_MICRO) && MAIX_HAS_TFLITE_MICRO
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/tools/benchmarking/op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

namespace {

constexpr size_t kMaixTflmArenaSize = 48 * 1024;

struct MaixTflmHandle {
  const unsigned char* model_data;
  unsigned char* owned_model_data;
  size_t model_size;
  unsigned char* tensor_arena;
  size_t tensor_arena_size;
  const tflite::Model* model;
  tflite::TflmOpResolver* resolver;
  tflite::MicroInterpreter* interpreter;
  TfLiteTensor* input_tensor;
  TfLiteTensor* output_tensor;
};

void SetError(char* error_buf, unsigned long error_buf_size,
              const char* message) {
  if (error_buf == nullptr || error_buf_size == 0) {
    return;
  }
  std::snprintf(error_buf, static_cast<size_t>(error_buf_size), "%s",
                message != nullptr ? message : "unknown");
}

void SetErrorFmt(char* error_buf, unsigned long error_buf_size,
                 const char* fmt, const char* value) {
  if (error_buf == nullptr || error_buf_size == 0) {
    return;
  }
  std::snprintf(error_buf, static_cast<size_t>(error_buf_size), fmt,
                value != nullptr ? value : "unknown");
}

bool LoadModelBytes(const char* model_path, MaixTflmHandle* handle,
                    char* error_buf, unsigned long error_buf_size) {
  const maix_bundle_file_t* bundle_file;
  size_t model_size = 0;
  size_t read_size = 0;

  bundle_file = maix_runtime_fs_find(model_path);
  if (bundle_file != nullptr) {
    handle->model_data = bundle_file->data;
    handle->model_size = bundle_file->size;
    return true;
  }

  if (maix_runtime_fs_file_size(model_path, &model_size) != 0 || model_size == 0) {
    SetErrorFmt(error_buf, error_buf_size, "failed to stat model: %s", model_path);
    return false;
  }

  handle->owned_model_data =
      static_cast<unsigned char*>(std::malloc(model_size));
  if (handle->owned_model_data == nullptr) {
    SetError(error_buf, error_buf_size, "out of memory for model buffer");
    return false;
  }

  if (maix_runtime_fs_read(model_path, 0, handle->owned_model_data, model_size,
                           &read_size) != 0 ||
      read_size != model_size) {
    SetErrorFmt(error_buf, error_buf_size, "failed to read model: %s", model_path);
    return false;
  }

  handle->model_data = handle->owned_model_data;
  handle->model_size = model_size;
  return true;
}

void FillTensorDefault(TfLiteTensor* tensor) {
  int fill_value = 0;

  if (tensor == nullptr || tensor->data.raw == nullptr || tensor->bytes == 0) {
    return;
  }

  if (tensor->type == kTfLiteInt8 || tensor->type == kTfLiteUInt8) {
    fill_value = tensor->params.zero_point;
  }

  std::memset(tensor->data.raw, fill_value, tensor->bytes);
}

void DestroyHandle(MaixTflmHandle* handle) {
  if (handle == nullptr) {
    return;
  }

  delete handle->interpreter;
  delete handle->resolver;
  std::free(handle->tensor_arena);
  std::free(handle->owned_model_data);
  delete handle;
}

}  // namespace
#endif

extern "C" int maix_tflm_backend_load(const char* model_path, void** handle,
                                       char* error_buf,
                                       unsigned long error_buf_size) {
#if defined(MAIX_HAS_TFLITE_MICRO) && MAIX_HAS_TFLITE_MICRO
  MaixTflmHandle* backend_handle;

  if (handle != nullptr) {
    *handle = nullptr;
  }

  if (model_path == nullptr || model_path[0] == '\0') {
    SetError(error_buf, error_buf_size, "missing model path");
    return -1;
  }

  backend_handle = new MaixTflmHandle();
  std::memset(backend_handle, 0, sizeof(*backend_handle));
  backend_handle->tensor_arena_size = kMaixTflmArenaSize;

  if (!LoadModelBytes(model_path, backend_handle, error_buf, error_buf_size)) {
    DestroyHandle(backend_handle);
    return -1;
  }

  backend_handle->model =
      tflite::GetModel(static_cast<const void*>(backend_handle->model_data));
  if (backend_handle->model == nullptr) {
    SetError(error_buf, error_buf_size, "invalid tflite model");
    DestroyHandle(backend_handle);
    return -1;
  }

  if (backend_handle->model->version() != TFLITE_SCHEMA_VERSION) {
    SetError(error_buf, error_buf_size, "unsupported tflite schema version");
    DestroyHandle(backend_handle);
    return -1;
  }

  backend_handle->resolver = new tflite::TflmOpResolver();
  if (backend_handle->resolver == nullptr ||
      tflite::CreateOpResolver(*backend_handle->resolver) != kTfLiteOk) {
    SetError(error_buf, error_buf_size, "failed to create tflm op resolver");
    DestroyHandle(backend_handle);
    return -1;
  }

  backend_handle->tensor_arena = static_cast<unsigned char*>(
      std::malloc(backend_handle->tensor_arena_size));
  if (backend_handle->tensor_arena == nullptr) {
    SetError(error_buf, error_buf_size, "out of memory for tensor arena");
    DestroyHandle(backend_handle);
    return -1;
  }
  std::memset(backend_handle->tensor_arena, 0, backend_handle->tensor_arena_size);

  backend_handle->interpreter = new tflite::MicroInterpreter(
      backend_handle->model, *backend_handle->resolver,
      backend_handle->tensor_arena, backend_handle->tensor_arena_size);
  if (backend_handle->interpreter == nullptr) {
    SetError(error_buf, error_buf_size, "failed to create micro interpreter");
    DestroyHandle(backend_handle);
    return -1;
  }

  if (backend_handle->interpreter->AllocateTensors() != kTfLiteOk) {
    SetError(error_buf, error_buf_size, "tflm AllocateTensors failed");
    DestroyHandle(backend_handle);
    return -1;
  }

  backend_handle->input_tensor = backend_handle->interpreter->input(0);
  backend_handle->output_tensor = backend_handle->interpreter->output(0);
  if (backend_handle->input_tensor == nullptr ||
      backend_handle->output_tensor == nullptr) {
    SetError(error_buf, error_buf_size, "missing input/output tensor");
    DestroyHandle(backend_handle);
    return -1;
  }

  if (handle != nullptr) {
    *handle = backend_handle;
  }
  SetError(error_buf, error_buf_size, "ok");
  return 0;
#else
  (void)model_path;
  if (handle != nullptr) {
    *handle = nullptr;
  }
  SetError(error_buf, error_buf_size, "tflite-micro not built");
  return -1;
#endif
}

extern "C" int maix_tflm_backend_forward(void* handle,
                                          const unsigned char* input,
                                          unsigned long input_size,
                                          unsigned char* output,
                                          unsigned long output_capacity,
                                          unsigned long* output_size,
                                          char* error_buf,
                                          unsigned long error_buf_size) {
#if defined(MAIX_HAS_TFLITE_MICRO) && MAIX_HAS_TFLITE_MICRO
  MaixTflmHandle* backend_handle = static_cast<MaixTflmHandle*>(handle);

  if (output_size != nullptr) {
    *output_size = 0;
  }

  if (backend_handle == nullptr || backend_handle->interpreter == nullptr ||
      backend_handle->input_tensor == nullptr ||
      backend_handle->output_tensor == nullptr) {
    SetError(error_buf, error_buf_size, "invalid tflm handle");
    return -1;
  }

  if (output == nullptr || output_capacity < backend_handle->output_tensor->bytes) {
    SetError(error_buf, error_buf_size, "output buffer too small");
    return -1;
  }

  FillTensorDefault(backend_handle->input_tensor);
  if (input != nullptr && input_size > 0) {
    if (input_size > backend_handle->input_tensor->bytes) {
      SetError(error_buf, error_buf_size, "input payload larger than tensor");
      return -1;
    }
    std::memcpy(backend_handle->input_tensor->data.raw, input, input_size);
  }

  if (backend_handle->interpreter->Invoke() != kTfLiteOk) {
    SetError(error_buf, error_buf_size, "tflm Invoke failed");
    return -1;
  }

  std::memcpy(output, backend_handle->output_tensor->data.raw,
              backend_handle->output_tensor->bytes);
  if (output_size != nullptr) {
    *output_size = backend_handle->output_tensor->bytes;
  }
  SetError(error_buf, error_buf_size, "ok");
  return 0;
#else
  (void)handle;
  (void)input;
  (void)input_size;
  (void)output;
  (void)output_capacity;
  if (output_size != nullptr) {
    *output_size = 0;
  }
  SetError(error_buf, error_buf_size, "tflite-micro not built");
  return -1;
#endif
}

extern "C" int maix_tflm_backend_unload(void* handle, char* error_buf,
                                         unsigned long error_buf_size) {
#if defined(MAIX_HAS_TFLITE_MICRO) && MAIX_HAS_TFLITE_MICRO
  DestroyHandle(static_cast<MaixTflmHandle*>(handle));
  SetError(error_buf, error_buf_size, "ok");
  return 0;
#else
  (void)handle;
  SetError(error_buf, error_buf_size, "tflite-micro not built");
  return -1;
#endif
}
