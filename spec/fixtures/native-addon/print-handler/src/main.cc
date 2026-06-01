#include <cstring>
#include <node_api.h>

#include "print_handler.h"

namespace {

// startWatching(action: string, timeoutMs?: number)
// Starts polling for a modal print dialog. Must be called BEFORE the dialog
// appears (i.e. before webContents.print()), because the modal dialog blocks
// the JS event loop.
napi_value StartWatching(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, NULL, NULL);

  char action[16];
  size_t action_len;
  napi_get_value_string_utf8(env, args[0], action, sizeof(action), &action_len);

  bool should_print = (strcmp(action, "print") == 0);

  int timeout_ms = 5000;
  if (argc >= 2) {
    napi_valuetype vtype;
    napi_typeof(env, args[1], &vtype);
    if (vtype == napi_number)
      napi_get_value_int32(env, args[1], &timeout_ms);
  }

  print_handler::StartWatching(should_print, timeout_ms);
  return NULL;
}

// stopWatching() → boolean
// Stops the poller and returns true if a dialog was successfully dismissed.
napi_value StopWatching(napi_env env, napi_callback_info info) {
  bool dismissed = print_handler::StopWatching();

  napi_value result;
  napi_get_boolean(env, dismissed, &result);
  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
      {"startWatching", NULL, StartWatching, NULL, NULL, NULL, napi_default,
       NULL},
      {"stopWatching", NULL, StopWatching, NULL, NULL, NULL, napi_default,
       NULL},
  };

  napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors);
  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
