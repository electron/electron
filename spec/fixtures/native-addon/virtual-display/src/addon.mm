#include <js_native_api.h>
#include <node_api.h>
#include "VirtualDisplayBridge.h"

namespace {

typedef struct {
  const char* name;
  int default_val;
  int* ptr;
} PropertySpec;

// Helper function to get an integer property from an object
bool GetIntProperty(napi_env env,
                    napi_value object,
                    const char* prop_name,
                    int* result,
                    int default_value) {
  *result = default_value;

  bool has_prop;
  if (napi_has_named_property(env, object, prop_name, &has_prop) != napi_ok ||
      !has_prop) {
    return true;
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

// Helper function to validate and parse object properties
bool ParseObjectProperties(napi_env env,
                           napi_value object,
                           PropertySpec props[],
                           size_t prop_count) {
  // Process all properties
  for (size_t i = 0; i < prop_count; i++) {
    if (!GetIntProperty(env, object, props[i].name, props[i].ptr,
                        props[i].default_val)) {
      char error_msg[50];
      snprintf(error_msg, sizeof(error_msg), "%s must be a number",
               props[i].name);
      napi_throw_error(env, NULL, error_msg);
      return false;
    }
  }

  // Check for unknown properties
  napi_value prop_names;
  uint32_t count;
  napi_get_property_names(env, object, &prop_names);
  napi_get_array_length(env, prop_names, &count);

  for (uint32_t i = 0; i < count; i++) {
    napi_value prop_name;
    napi_get_element(env, prop_names, i, &prop_name);
    size_t len;
    char name[20];
    napi_get_value_string_utf8(env, prop_name, name, sizeof(name), &len);

    bool found = false;
    for (size_t j = 0; j < prop_count; j++) {
      if (strcmp(name, props[j].name) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      napi_throw_error(env, NULL, "Object contains unknown properties");
      return false;
    }
  }

  return true;
}

// virtualDisplay.create()
napi_value create(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];

  if (napi_get_cb_info(env, info, &argc, args, NULL, NULL) != napi_ok) {
    return NULL;
  }

  int width = 1920, height = 1080, x = 0, y = 0;

  PropertySpec props[] = {{"width", 1920, &width},
                          {"height", 1080, &height},
                          {"x", 0, &x},
                          {"y", 0, &y}};

  if (argc >= 1) {
    napi_valuetype valuetype;
    if (napi_typeof(env, args[0], &valuetype) != napi_ok) {
      napi_throw_error(env, NULL, "Failed to get argument type");
      return NULL;
    }

    if (valuetype == napi_object) {
      if (!ParseObjectProperties(env, args[0], props,
                                 sizeof(props) / sizeof(props[0]))) {
        return NULL;
      }
    } else {
      napi_throw_error(env, NULL, "Expected an object as the argument");
      return NULL;
    }
  }

  NSInteger displayId = [VirtualDisplayBridge create:width
                                              height:height
                                                   x:x
                                                   y:y];

  if (displayId == 0) {
    napi_throw_error(env, NULL, "Failed to create virtual display");
    return NULL;
  }

  napi_value result;
  if (napi_create_int64(env, displayId, &result) != napi_ok) {
    return NULL;
  }

  return result;
}

// virtualDisplay.forceCleanup()
napi_value forceCleanup(napi_env env, napi_callback_info info) {
  BOOL result = [VirtualDisplayBridge forceCleanup];

  napi_value js_result;
  if (napi_get_boolean(env, result, &js_result) != napi_ok) {
    return NULL;
  }

  return js_result;
}

// virtualDisplay.destroy()
napi_value destroy(napi_env env, napi_callback_info info) {
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

  BOOL result = [VirtualDisplayBridge destroy:(NSInteger)displayId];

  napi_value js_result;
  if (napi_get_boolean(env, result, &js_result) != napi_ok) {
    return NULL;
  }

  return js_result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
      {"create", NULL, create, NULL, NULL, NULL, napi_default, NULL},
      {"destroy", NULL, destroy, NULL, NULL, NULL, napi_default, NULL},
      {"forceCleanup", NULL, forceCleanup, NULL, NULL, NULL, napi_default,
       NULL}};
      {"destroy", NULL, destroy, NULL, NULL, NULL, napi_default, NULL}};

  if (napi_define_properties(env, exports,
                             sizeof(descriptors) / sizeof(*descriptors),
                             descriptors) != napi_ok) {
    return NULL;
  }

  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)