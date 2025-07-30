#include <js_native_api.h>
#include <node_api.h>
#include "VirtualDisplayBridge.h"

namespace {

napi_value AddDisplay(napi_env env, napi_callback_info info) {
  NSInteger displayId = [VirtualDisplayBridge addDisplay];

  napi_value result;
  napi_status status = napi_create_int64(env, displayId, &result);
  if (status != napi_ok)
    return NULL;

  return result;
}

napi_value RemoveDisplay(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  if (status != napi_ok)
    return NULL;

  if (argc < 1) {
    napi_throw_error(env, NULL, "Expected number argument");
    return NULL;
  }

  int64_t displayId;
  status = napi_get_value_int64(env, args[0], &displayId);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Expected number argument");
    return NULL;
  }

  BOOL result = [VirtualDisplayBridge removeDisplay:(NSInteger)displayId];

  napi_value js_result;
  status = napi_get_boolean(env, result, &js_result);
  if (status != napi_ok)
    return NULL;

  return js_result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor descriptors[] = {
      {"addDisplay", NULL, AddDisplay, NULL, NULL, NULL, napi_default, NULL},
      {"removeDisplay", NULL, RemoveDisplay, NULL, NULL, NULL, napi_default,
       NULL}};

  status = napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors);
  if (status != napi_ok)
    return NULL;

  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)