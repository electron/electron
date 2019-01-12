#include <js_native_api.h>
#include <node_api.h>

namespace {

napi_value Print(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  if (status != napi_ok)
    return NULL;

  if (argc > 1) {
    napi_throw_error(env, NULL,
                     "Wrong number of arguments, expected single argument");
  }

  return args[0];
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor descriptors[] = {
      {"Print", NULL, Print, NULL, NULL, NULL, napi_default, NULL}};

  status = napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors);
  if (status != napi_ok)
    return NULL;

  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
