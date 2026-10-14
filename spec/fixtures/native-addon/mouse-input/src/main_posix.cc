// N-API surface of the addon on macOS and Linux, matching main_win.cc: the
// same functions with the same result shapes, backed by impl_mac.mm
// (CGEventPost) or impl_linux.cc (XTEST).

#include <js_native_api.h>
#include <node_api.h>

#include <string>

#include "impl.h"

namespace {

#define NAPI_CALL(env, call)                                         \
  do {                                                               \
    if ((call) != napi_ok) {                                         \
      napi_throw_error((env), nullptr, "N-API call failed: " #call); \
      return nullptr;                                                \
    }                                                                \
  } while (0)

void SetInt(napi_env env, napi_value obj, const char* key, int64_t value) {
  napi_value v;
  napi_create_int64(env, value, &v);
  napi_set_named_property(env, obj, key, v);
}

void SetBool(napi_env env, napi_value obj, const char* key, bool value) {
  napi_value v;
  napi_get_boolean(env, value, &v);
  napi_set_named_property(env, obj, key, v);
}

void SetString(napi_env env,
               napi_value obj,
               const char* key,
               const std::string& value) {
  napi_value v;
  napi_create_string_utf8(env, value.c_str(), value.size(), &v);
  napi_set_named_property(env, obj, key, v);
}

bool GetInt32Arg(napi_env env, napi_value value, int32_t* out) {
  return napi_get_value_int32(env, value, out) == napi_ok;
}

bool GetStringArg(napi_env env, napi_value value, std::string* out) {
  size_t length = 0;
  if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok)
    return false;
  out->resize(length);
  size_t copied = 0;
  if (napi_get_value_string_utf8(env, value, &(*out)[0], length + 1, &copied) !=
      napi_ok)
    return false;
  out->resize(copied);
  return true;
}

// ---------------------------------------------------------------------------
// Injection runs on a libuv worker thread, like SendInput() on Windows, so the
// main thread keeps pumping while the OS processes the input.

enum class Op { kMove, kButton, kWheel };

struct InjectWork {
  napi_async_work work = nullptr;
  napi_deferred deferred = nullptr;
  Op op = Op::kMove;
  int x = 0;
  int y = 0;
  mouse_input::Button button = mouse_input::Button::kLeft;
  bool down = false;
  bool up = false;
  int delta = 0;
  bool horizontal = false;
  mouse_input::InjectResult result;
};

void Accumulate(mouse_input::InjectResult* total,
                const mouse_input::InjectResult& part) {
  total->sent += part.sent;
  total->expected += part.expected;
  if (part.error)
    total->error = part.error;
}

void ExecuteInject(napi_env env, void* data) {
  auto* work = static_cast<InjectWork*>(data);
  switch (work->op) {
    case Op::kMove:
      work->result = mouse_input::Move(work->x, work->y);
      break;
    case Op::kButton:
      if (work->down)
        Accumulate(&work->result, mouse_input::Press(work->button, true));
      if (work->up)
        Accumulate(&work->result, mouse_input::Press(work->button, false));
      break;
    case Op::kWheel:
      work->result = mouse_input::Wheel(work->delta, work->horizontal);
      break;
  }
}

void CompleteInject(napi_env env, napi_status status, void* data) {
  auto* work = static_cast<InjectWork*>(data);
  napi_value result;
  napi_create_object(env, &result);
  SetInt(env, result, "sent", work->result.sent);
  SetInt(env, result, "expected", work->result.expected);
  SetInt(env, result, "error", work->result.error);
  napi_resolve_deferred(env, work->deferred, result);
  napi_delete_async_work(env, work->work);
  delete work;
}

napi_value QueueInject(napi_env env, InjectWork* work) {
  napi_value promise;
  napi_value name;
  napi_create_promise(env, &work->deferred, &promise);
  napi_create_string_utf8(env, "mouseInputInject", NAPI_AUTO_LENGTH, &name);
  napi_create_async_work(env, nullptr, name, ExecuteInject, CompleteInject,
                         work, &work->work);
  napi_queue_async_work(env, work->work);
  return promise;
}

// move(x, y): absolute move to screen point (x, y).
napi_value MoveJs(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  int32_t x = 0, y = 0;
  if (argc < 2 || !GetInt32Arg(env, args[0], &x) ||
      !GetInt32Arg(env, args[1], &y)) {
    napi_throw_type_error(env, nullptr, "move(x, y) expects two integers");
    return nullptr;
  }
  auto* work = new InjectWork();
  work->op = Op::kMove;
  work->x = x;
  work->y = y;
  return QueueInject(env, work);
}

// button(button, action): button is 'left' | 'right' | 'middle', action is
// 'down' | 'up' | 'click'. Presses at the current cursor position.
napi_value ButtonJs(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  std::string button, action;
  if (argc < 2 || !GetStringArg(env, args[0], &button) ||
      !GetStringArg(env, args[1], &action)) {
    napi_throw_type_error(env, nullptr,
                          "button(button, action) expects two strings");
    return nullptr;
  }
  mouse_input::Button which;
  if (button == "left") {
    which = mouse_input::Button::kLeft;
  } else if (button == "right") {
    which = mouse_input::Button::kRight;
  } else if (button == "middle") {
    which = mouse_input::Button::kMiddle;
  } else {
    napi_throw_range_error(env, nullptr, "unknown mouse button");
    return nullptr;
  }
  const bool down = action == "down" || action == "click";
  const bool up = action == "up" || action == "click";
  if (!down && !up) {
    napi_throw_range_error(env, nullptr, "action must be down, up or click");
    return nullptr;
  }
  auto* work = new InjectWork();
  work->op = Op::kButton;
  work->button = which;
  work->down = down;
  work->up = up;
  return QueueInject(env, work);
}

// wheel(delta, horizontal?): delta in WHEEL_DELTA (120) units per notch.
napi_value WheelJs(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  int32_t delta = 0;
  if (argc < 1 || !GetInt32Arg(env, args[0], &delta)) {
    napi_throw_type_error(env, nullptr, "wheel(delta) expects an integer");
    return nullptr;
  }
  bool horizontal = false;
  if (argc >= 2)
    napi_get_value_bool(env, args[1], &horizontal);
  auto* work = new InjectWork();
  work->op = Op::kWheel;
  work->delta = delta;
  work->horizontal = horizontal;
  return QueueInject(env, work);
}

// getCursorPos(): { x, y }, or { error } when the position is unavailable.
napi_value GetCursorPosJs(napi_env env, napi_callback_info info) {
  napi_value result;
  NAPI_CALL(env, napi_create_object(env, &result));
  int x = 0, y = 0;
  int64_t error = 0;
  if (!mouse_input::GetCursorPos(&x, &y, &error)) {
    SetInt(env, result, "error", error);
    return result;
  }
  SetInt(env, result, "x", x);
  SetInt(env, result, "y", y);
  return result;
}

#if defined(__APPLE__)
// postClickToWindow(pid, windowNumber, x, y): see impl.h.
napi_value PostClickToWindowJs(napi_env env, napi_callback_info info) {
  size_t argc = 4;
  napi_value args[4];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  int32_t pid = 0, window_number = 0, x = 0, y = 0;
  if (argc < 4 || !GetInt32Arg(env, args[0], &pid) ||
      !GetInt32Arg(env, args[1], &window_number) ||
      !GetInt32Arg(env, args[2], &x) || !GetInt32Arg(env, args[3], &y)) {
    napi_throw_type_error(
        env, nullptr,
        "postClickToWindow(pid, windowNumber, x, y) expects four integers");
    return nullptr;
  }
  const mouse_input::InjectResult posted =
      mouse_input::PostClickToWindow(pid, window_number, x, y);
  napi_value result;
  NAPI_CALL(env, napi_create_object(env, &result));
  SetInt(env, result, "sent", posted.sent);
  SetInt(env, result, "expected", posted.expected);
  SetInt(env, result, "error", posted.error);
  return result;
}
#endif

// isWindowAtPoint(handle, x, y): see impl.h. Throws where unsupported.
napi_value IsWindowAtPointJs(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  bool is_buffer = false;
  void* data = nullptr;
  size_t length = 0;
  int32_t x = 0, y = 0;
  if (argc < 3 || napi_is_buffer(env, args[0], &is_buffer) != napi_ok ||
      !is_buffer ||
      napi_get_buffer_info(env, args[0], &data, &length) != napi_ok ||
      !GetInt32Arg(env, args[1], &x) || !GetInt32Arg(env, args[2], &y)) {
    napi_throw_type_error(env, nullptr,
                          "isWindowAtPoint(handle, x, y) expects a native "
                          "window handle Buffer and two integers");
    return nullptr;
  }
  bool supported = true;
  const bool hit = mouse_input::IsWindowAtPoint(data, length, x, y, &supported);
  if (!supported) {
    napi_throw_error(env, nullptr,
                     "isWindowAtPoint() is not supported on this platform");
    return nullptr;
  }
  napi_value result;
  napi_get_boolean(env, hit, &result);
  return result;
}

napi_value GetDiagnosticsJs(napi_env env, napi_callback_info info) {
  napi_value result;
  NAPI_CALL(env, napi_create_object(env, &result));
  for (const auto& [key, value] : mouse_input::GetDiagnostics()) {
    switch (value.type) {
      case mouse_input::DiagnosticValue::Type::kString:
        SetString(env, result, key.c_str(), value.string_value);
        break;
      case mouse_input::DiagnosticValue::Type::kInt:
        SetInt(env, result, key.c_str(), value.int_value);
        break;
      case mouse_input::DiagnosticValue::Type::kBool:
        SetBool(env, result, key.c_str(), value.bool_value);
        break;
    }
  }
  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
      {"move", nullptr, MoveJs, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"button", nullptr, ButtonJs, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"wheel", nullptr, WheelJs, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"getCursorPos", nullptr, GetCursorPosJs, nullptr, nullptr, nullptr,
       napi_default, nullptr},
#if defined(__APPLE__)
      {"postClickToWindow", nullptr, PostClickToWindowJs, nullptr, nullptr,
       nullptr, napi_default, nullptr},
#endif
      {"isWindowAtPoint", nullptr, IsWindowAtPointJs, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"getDiagnostics", nullptr, GetDiagnosticsJs, nullptr, nullptr, nullptr,
       napi_default, nullptr},
  };
  napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
