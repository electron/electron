#include <js_native_api.h>
#include <node_api.h>
#include "VirtualDisplayBridge.h"

namespace {

// Helper function to extract integer property from object
bool GetIntProperty(napi_env env,
                    napi_value object,
                    const char* prop_name,
                    int* result,
                    int default_value) {
  *result = default_value;

  bool has_prop;
  if (napi_has_named_property(env, object, prop_name, &has_prop) != napi_ok ||
      !has_prop) {
    return true;  // Use default
  }

  napi_value prop_value;
  if (napi_get_named_property(env, object, prop_name, &prop_value) != napi_ok) {
    return false;
  }

  if (napi_get_value_int32(env, prop_value, result) != napi_ok) {
    return false;
  }

  return true;
}

napi_value AddDisplay(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];

  if (napi_get_cb_info(env, info, &argc, args, NULL, NULL) != napi_ok) {
    return NULL;
  }

  // Default values
  int width = 1920;
  int height = 1080;

  // If object argument provided, extract properties
  if (argc >= 1) {
    napi_valuetype valuetype;
    if (napi_typeof(env, args[0], &valuetype) != napi_ok) {
      napi_throw_error(env, NULL, "Failed to get argument type");
      return NULL;
    }

    if (valuetype == napi_object) {
      if (!GetIntProperty(env, args[0], "width", &width, 1920)) {
        napi_throw_error(env, NULL, "Width must be a number");
        return NULL;
      }

      if (!GetIntProperty(env, args[0], "height", &height, 1080)) {
        napi_throw_error(env, NULL, "Height must be a number");
        return NULL;
      }
    } else {
      napi_throw_error(env, NULL,
                       "Expected object with width/height properties");
      return NULL;
    }
  }

  NSInteger displayId = [VirtualDisplayBridge addDisplay:width height:height];

  napi_value result;
  if (napi_create_int64(env, displayId, &result) != napi_ok) {
    return NULL;
  }

  return result;
}

napi_value RemoveDisplay(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];

  if (napi_get_cb_info(env, info, &argc, args, NULL, NULL) != napi_ok) {
    return NULL;
  }

  if (argc < 1) {
    napi_throw_error(env, NULL, "Expected number argument");
    return NULL;
  }

  int64_t displayId;
  if (napi_get_value_int64(env, args[0], &displayId) != napi_ok) {
    napi_throw_error(env, NULL, "Expected number argument");
    return NULL;
  }

  BOOL result = [VirtualDisplayBridge removeDisplay:(NSInteger)displayId];

  napi_value js_result;
  if (napi_get_boolean(env, result, &js_result) != napi_ok) {
    return NULL;
  }

  return js_result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
      {"addDisplay", NULL, AddDisplay, NULL, NULL, NULL, napi_default, NULL},
      {"removeDisplay", NULL, RemoveDisplay, NULL, NULL, NULL, napi_default,
       NULL}};

  if (napi_define_properties(env, exports,
                             sizeof(descriptors) / sizeof(*descriptors),
                             descriptors) != napi_ok) {
    return NULL;
  }

  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)