#include <napi.h>
#include <uv.h>

namespace test_module {

Napi::Value TestLoadLibrary(const Napi::CallbackInfo& info) {
  auto lib_path = info[0].ToString().Utf8Value();
  uv_lib_t lib;
  auto result = uv_dlopen(lib_path.c_str(), &lib);
  if (result == 0) {
    return Napi::Value::From(info.Env(), true);
  } else {
    Napi::Error::New(info.Env(), uv_dlerror(&lib)).ThrowAsJavaScriptException();
    return Napi::Value();
  }
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return Napi::Function::New(env, TestLoadLibrary);
}

NODE_API_MODULE(TestLoadLibrary, Init);

}  // namespace test_module