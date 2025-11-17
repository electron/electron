#include <js_native_api.h>
#include <node_api.h>

#include "impl.h"

namespace {

napi_value IsValidWindow(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1], result;
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  if (status != napi_ok)
    return NULL;

  bool is_buffer;
  status = napi_is_buffer(env, args[0], &is_buffer);
  if (status != napi_ok)
    return NULL;

  if (!is_buffer) {
    napi_throw_error(env, NULL, "First argument must be Buffer");
    return NULL;
  }

  char* data;
  size_t length;
  status = napi_get_buffer_info(env, args[0], (void**)&data, &length);
  if (status != napi_ok)
    return NULL;

  status = napi_get_boolean(env, impl::IsValidWindow(data, length), &result);
  if (status != napi_ok)
    return NULL;

  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor descriptors[] = {{"isValidWindow", NULL,
                                             IsValidWindow, NULL, NULL, NULL,
                                             napi_default, NULL}};

  status = napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors);
  if (status != napi_ok)
    return NULL;

  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
