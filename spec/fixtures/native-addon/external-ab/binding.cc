#include <memory>

#include <node_api.h>
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

  auto deleter = [](void* data, size_t length, void* deleter_data) {
    delete[] static_cast<uint8_t*>(data);
  };

  // NOTE: the V8 API is invoked directly, rather than node::Buffer::New() or
  // its napi equivalent, to trigger the FATAL error from V8: memory allocated
  // outside the V8 sandbox cannot back an ArrayBuffer. Since Node.js 26,
  // node::Buffer::New() copies such memory into the sandbox instead.
  std::unique_ptr<v8::BackingStore> backing_store =
      v8::ArrayBuffer::NewBackingStore(data, length, deleter, nullptr);
  v8::Local<v8::ArrayBuffer> array_buffer =
      v8::ArrayBuffer::New(isolate, std::move(backing_store));

  return reinterpret_cast<napi_value>(*array_buffer);
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
