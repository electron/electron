#include <node_api.h>
#include <uv.h>

namespace test_module {

napi_value TestLoadLibrary(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv;
  napi_status status;
  status = napi_get_cb_info(env, info, &argc, &argv, NULL, NULL);
  if (status != napi_ok)
    napi_fatal_error(NULL, 0, NULL, 0);

  char lib_path[256];
  status = napi_get_value_string_utf8(env, argv, lib_path, 256, NULL);
  if (status != napi_ok)
    napi_fatal_error(NULL, 0, NULL, 0);

  uv_lib_t lib;
  auto uv_status = uv_dlopen(lib_path, &lib);
  if (uv_status == 0) {
    napi_value result;
    status = napi_get_boolean(env, true, &result);
    if (status != napi_ok)
      napi_fatal_error(NULL, 0, NULL, 0);
    return result;
  } else {
    status = napi_throw_error(env, NULL, uv_dlerror(&lib));
    if (status != napi_ok)
      napi_fatal_error(NULL, 0, NULL, 0);
  }
}

napi_value Init(napi_env env, napi_value exports) {
  napi_value method;
  napi_status status;
  status = napi_create_function(env, "testLoadLibrary", NAPI_AUTO_LENGTH,
                                TestLoadLibrary, NULL, &method);
  if (status != napi_ok)
    return NULL;
  return method;
}

NAPI_MODULE(TestLoadLibrary, Init);

}  // namespace test_module