#include <node_api.h>
#undef NAPI_VERSION
#include <node_buffer.h>
#include <v8.h>

namespace {

napi_value CreateBuffer(napi_env env, napi_callback_info info) {
  v8::Isolate* isolate = v8::Isolate::TryGetCurrent();
  if (isolate == nullptr) {
    return NULL;
  }

  const size_t length = 4;

  uint8_t* data = new uint8_t[length];
  for (size_t i = 0; i < 4; i++) {
    data[i] = static_cast<uint8_t>(length);
  }

  auto finalizer = [](char* data, void* hint) {
    delete[] static_cast<uint8_t*>(reinterpret_cast<void*>(data));
  };

  // NOTE: Buffer API is invoked directly rather than
  // napi version to trigger the FATAL error from V8.
  v8::MaybeLocal<v8::Object> maybe = node::Buffer::New(
      isolate, static_cast<char*>(reinterpret_cast<void*>(data)), length,
      finalizer, nullptr);

  return reinterpret_cast<napi_value>(*maybe.ToLocalChecked());
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor descriptors[] = {{"createBuffer", NULL, CreateBuffer,
                                             NULL, NULL, NULL, napi_default,
                                             NULL}};

  status = napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors);
  if (status != napi_ok)
    return NULL;

  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
