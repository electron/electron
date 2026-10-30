#include <memory>

#include <node_api.h>
#include <v8.h>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

#if defined(_WIN32)
// Electron is built with Chromium's libc++, which puts the standard library in
// std::__Cr, and MSVC mangles a function's return type into its name. So the
// v8::ArrayBuffer::NewBackingStore() that electron.exe exports returns a
// std::__Cr::unique_ptr, and the name this addon's MSVC STL headers produce
// for it does not resolve. Look it up by Electron's name instead.
constexpr char kNewBackingStoreName[] =
    "?NewBackingStore@ArrayBuffer@v8@@SA?AV?$unique_ptr@VBackingStore@v8@@U?$"
    "default_delete@VBackingStore@v8@@@__Cr@std@@@__Cr@std@@PEAX_KP6AX010@Z0@"
    "Z";

// libc++'s std::unique_ptr<v8::BackingStore> is a single pointer. It has a
// non-trivial destructor, so on x64 the caller passes the return slot as a
// hidden first argument.
struct BackingStoreSlot {
  void* store;
};
using NewBackingStoreFn =
    BackingStoreSlot* (*)(BackingStoreSlot*,
                          void*,
                          size_t,
                          v8::BackingStore::DeleterCallback,
                          void*);
#endif

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
#if defined(_WIN32)
  auto new_backing_store = reinterpret_cast<NewBackingStoreFn>(
      GetProcAddress(GetModuleHandleW(nullptr), kNewBackingStoreName));
  if (new_backing_store == nullptr) {
    delete[] data;
    napi_throw_error(env, nullptr,
                     "v8::ArrayBuffer::NewBackingStore is not exported");
    return NULL;
  }
  BackingStoreSlot slot;
  new_backing_store(&slot, data, length, deleter, nullptr);
#else
  // Only call V8 exports without standard library types in their mangled
  // names: Electron's libc++ (std::__Cr) does not match the one this addon is
  // built with, and the return type is not part of the name here.
  std::unique_ptr<v8::BackingStore> backing_store =
      v8::ArrayBuffer::NewBackingStore(data, length, deleter, nullptr);
#endif
  // Not reached: V8 raises a fatal error for the backing store above.
  napi_throw_error(env, nullptr,
                   "v8::ArrayBuffer::NewBackingStore did not raise a fatal "
                   "error");
  return NULL;
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
