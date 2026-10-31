// Test-only helper that drives the real Windows input stack from specs.
//
// Everything here goes through SendInput(), so events are injected into the
// system input queue exactly like a physical mouse: they are seen by low level
// mouse hooks (WH_MOUSE_LL, with LLMHF_INJECTED set), hit tested against the
// window under the cursor, and delivered as WM_MOUSEMOVE / WM_*BUTTON* /
// WM_MOUSEWHEEL / WM_NC* messages. This is what webContents.sendInputEvent()
// cannot do, since that feeds Blink directly and never touches Win32.
//
// All coordinates are physical screen pixels (use screen.dipToScreenPoint()).

#include <js_native_api.h>
#include <node_api.h>

#include <windows.h>

#include <cstring>
#include <string>

namespace {

#define NAPI_CALL(env, call)                                         \
  do {                                                               \
    if ((call) != napi_ok) {                                         \
      napi_throw_error((env), nullptr, "N-API call failed: " #call); \
      return nullptr;                                                \
    }                                                                \
  } while (0)

// ---------------------------------------------------------------------------
// Small N-API helpers.

napi_value MakeInt(napi_env env, int64_t value) {
  napi_value result;
  napi_create_int64(env, value, &result);
  return result;
}

napi_value MakeBool(napi_env env, bool value) {
  napi_value result;
  napi_get_boolean(env, value, &result);
  return result;
}

napi_value MakeString(napi_env env, const std::string& value) {
  napi_value result;
  napi_create_string_utf8(env, value.c_str(), value.size(), &result);
  return result;
}

void SetInt(napi_env env, napi_value obj, const char* key, int64_t value) {
  napi_set_named_property(env, obj, key, MakeInt(env, value));
}

void SetBool(napi_env env, napi_value obj, const char* key, bool value) {
  napi_set_named_property(env, obj, key, MakeBool(env, value));
}

void SetString(napi_env env,
               napi_value obj,
               const char* key,
               const std::string& value) {
  napi_set_named_property(env, obj, key, MakeString(env, value));
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

// The buffer returned by BrowserWindow.getNativeWindowHandle().
bool GetHwndArg(napi_env env, napi_value value, HWND* out) {
  bool is_buffer = false;
  if (napi_is_buffer(env, value, &is_buffer) != napi_ok || !is_buffer)
    return false;
  void* data = nullptr;
  size_t length = 0;
  if (napi_get_buffer_info(env, value, &data, &length) != napi_ok ||
      length != sizeof(HWND))
    return false;
  std::memcpy(out, data, sizeof(HWND));
  return true;
}

std::string WideToUtf8(const wchar_t* wide) {
  int size =
      ::WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
  if (size <= 1)
    return std::string();
  std::string result(size - 1, '\0');
  ::WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], size, nullptr,
                        nullptr);
  return result;
}

// Name of a desktop or window station handle, e.g. "Default" or "Winlogon"
// (secure desktop), "WinSta0" (interactive window station).
std::string ObjectName(HANDLE desktop) {
  wchar_t name[256] = {0};
  DWORD needed = 0;
  if (!::GetUserObjectInformationW(desktop, UOI_NAME, name, sizeof(name),
                                   &needed))
    return std::string();
  return WideToUtf8(name);
}

// ---------------------------------------------------------------------------
// SendInput plumbing. Injection runs on a libuv worker thread so that the
// Electron UI thread -- which owns the WH_MOUSE_LL hook installed by
// setIgnoreMouseEvents(true, { forward: true }) -- is free to service the hook
// while the input is being processed.

constexpr int kMaxInputs = 4;

struct SendInputWork {
  napi_async_work work = nullptr;
  napi_deferred deferred = nullptr;
  INPUT inputs[kMaxInputs] = {};
  UINT count = 0;
  UINT sent = 0;
  DWORD error = 0;
};

void ExecuteSendInput(napi_env env, void* data) {
  auto* work = static_cast<SendInputWork*>(data);
  ::SetLastError(0);
  work->sent = ::SendInput(work->count, work->inputs, sizeof(INPUT));
  work->error = work->sent == work->count ? 0 : ::GetLastError();
}

void CompleteSendInput(napi_env env, napi_status status, void* data) {
  auto* work = static_cast<SendInputWork*>(data);
  napi_value result;
  napi_create_object(env, &result);
  SetInt(env, result, "sent", work->sent);
  SetInt(env, result, "expected", work->count);
  // 5 == ERROR_ACCESS_DENIED: typically UIPI (target at a higher integrity
  // level) or the calling thread's desktop is not the input desktop (locked
  // workstation, secure desktop, session 0 / service without a desktop).
  SetInt(env, result, "error", work->error);
  napi_resolve_deferred(env, work->deferred, result);
  napi_delete_async_work(env, work->work);
  delete work;
}

napi_value QueueSendInput(napi_env env, SendInputWork* work) {
  napi_value promise;
  napi_value name;
  napi_create_promise(env, &work->deferred, &promise);
  napi_create_string_utf8(env, "mouseInputSendInput", NAPI_AUTO_LENGTH, &name);
  napi_create_async_work(env, nullptr, name, ExecuteSendInput,
                         CompleteSendInput, work, &work->work);
  napi_queue_async_work(env, work->work);
  return promise;
}

INPUT MouseInput(DWORD flags, LONG dx = 0, LONG dy = 0, DWORD data = 0) {
  INPUT input = {};
  input.type = INPUT_MOUSE;
  input.mi.dx = dx;
  input.mi.dy = dy;
  input.mi.mouseData = data;
  input.mi.dwFlags = flags;
  return input;
}

// Normalizes a physical screen coordinate to the 0..65535 range that
// MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK expects, aiming at the
// centre of the pixel so rounding inside win32k lands on |pos|.
LONG Normalize(int pos, int origin, int extent) {
  if (extent <= 0)
    return 0;
  return static_cast<LONG>(
      ((static_cast<int64_t>(pos - origin) * 2 + 1) * 65536) / (extent * 2));
}

// move(x, y): absolute move to physical screen pixel (x, y).
napi_value Move(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  int32_t x = 0, y = 0;
  if (argc < 2 || !GetInt32Arg(env, args[0], &x) ||
      !GetInt32Arg(env, args[1], &y)) {
    napi_throw_type_error(env, nullptr, "move(x, y) expects two integers");
    return nullptr;
  }
  const int vx = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
  const int vy = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
  const int vw = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
  const int vh = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

  auto* work = new SendInputWork();
  work->inputs[0] = MouseInput(
      MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK,
      Normalize(x, vx, vw), Normalize(y, vy, vh));
  work->count = 1;
  return QueueSendInput(env, work);
}

bool ButtonFlags(const std::string& button, bool down, DWORD* flags) {
  if (button == "left")
    *flags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
  else if (button == "right")
    *flags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
  else if (button == "middle")
    *flags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
  else
    return false;
  return true;
}

// button(button, action): button is 'left' | 'right' | 'middle', action is
// 'down' | 'up' | 'click'. Presses at the current cursor position.
napi_value Button(napi_env env, napi_callback_info info) {
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
  DWORD down_flags = 0, up_flags = 0;
  if (!ButtonFlags(button, true, &down_flags) ||
      !ButtonFlags(button, false, &up_flags)) {
    napi_throw_range_error(env, nullptr, "unknown mouse button");
    return nullptr;
  }
  auto* work = new SendInputWork();
  if (action == "down" || action == "click")
    work->inputs[work->count++] = MouseInput(down_flags);
  if (action == "up" || action == "click")
    work->inputs[work->count++] = MouseInput(up_flags);
  if (work->count == 0) {
    delete work;
    napi_throw_range_error(env, nullptr, "action must be down, up or click");
    return nullptr;
  }
  return QueueSendInput(env, work);
}

// wheel(delta, horizontal?): delta in WHEEL_DELTA (120) units per notch.
napi_value Wheel(napi_env env, napi_callback_info info) {
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
  auto* work = new SendInputWork();
  work->inputs[0] =
      MouseInput(horizontal ? MOUSEEVENTF_HWHEEL : MOUSEEVENTF_WHEEL, 0, 0,
                 static_cast<DWORD>(delta));
  work->count = 1;
  return QueueSendInput(env, work);
}

// ---------------------------------------------------------------------------
// Synchronous helpers.

// getCursorPos(): { x, y } in physical pixels, or null (with the error code)
// when the calling thread's desktop is not the input desktop.
napi_value GetCursorPosition(napi_env env, napi_callback_info info) {
  POINT point = {};
  napi_value result;
  napi_create_object(env, &result);
  if (!::GetCursorPos(&point)) {
    SetInt(env, result, "error", ::GetLastError());
    return result;
  }
  SetInt(env, result, "x", point.x);
  SetInt(env, result, "y", point.y);
  return result;
}

// isWindowAtPoint(hwnd, x, y): whether ::WindowFromPoint() at the physical
// screen point resolves to |hwnd| or one of its children. A window made
// click-through by setIgnoreMouseEvents(true) is expected to report false.
napi_value IsWindowAtPoint(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  HWND hwnd = nullptr;
  int32_t x = 0, y = 0;
  if (argc < 3 || !GetHwndArg(env, args[0], &hwnd) ||
      !GetInt32Arg(env, args[1], &x) || !GetInt32Arg(env, args[2], &y)) {
    napi_throw_type_error(env, nullptr,
                          "isWindowAtPoint(handle, x, y) expects a native "
                          "window handle Buffer and two integers");
    return nullptr;
  }
  POINT point = {x, y};
  HWND hit = ::WindowFromPoint(point);
  return MakeBool(env, hit && (hit == hwnd || ::IsChild(hwnd, hit) ||
                               ::GetAncestor(hit, GA_ROOT) == hwnd));
}

// describeWindowAtPoint(x, y): what ::WindowFromPoint() hit tests at the
// physical screen point, for failure messages: the top level window's class,
// title, owning process and extended style.
napi_value DescribeWindowAtPoint(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  int32_t x = 0, y = 0;
  if (argc < 2 || !GetInt32Arg(env, args[0], &x) ||
      !GetInt32Arg(env, args[1], &y)) {
    napi_throw_type_error(env, nullptr,
                          "describeWindowAtPoint(x, y) expects two integers");
    return nullptr;
  }
  napi_value result;
  NAPI_CALL(env, napi_create_object(env, &result));
  POINT point = {x, y};
  HWND hit = ::WindowFromPoint(point);
  if (!hit)
    return result;
  HWND root = ::GetAncestor(hit, GA_ROOT);
  if (!root)
    root = hit;
  SetInt(env, result, "hwnd", reinterpret_cast<intptr_t>(root));
  wchar_t text[256] = {0};
  if (::GetClassNameW(root, text, 256))
    SetString(env, result, "className", WideToUtf8(text));
  text[0] = 0;
  ::GetWindowTextW(root, text, 256);
  SetString(env, result, "title", WideToUtf8(text));
  const LONG ex_style = ::GetWindowLongW(root, GWL_EXSTYLE);
  SetInt(env, result, "exStyle", ex_style);
  SetBool(env, result, "topmost", (ex_style & WS_EX_TOPMOST) != 0);
  DWORD pid = 0;
  ::GetWindowThreadProcessId(root, &pid);
  SetInt(env, result, "pid", pid);
  SetBool(env, result, "ownProcess", pid == ::GetCurrentProcessId());
  HANDLE process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (process) {
    wchar_t image[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    if (::QueryFullProcessImageNameW(process, 0, image, &size))
      SetString(env, result, "process", WideToUtf8(image));
    ::CloseHandle(process);
  }
  return result;
}

// getDiagnostics(): everything needed to tell from a CI log whether real input
// injection can work on this machine.
napi_value GetDiagnostics(napi_env env, napi_callback_info info) {
  napi_value result;
  NAPI_CALL(env, napi_create_object(env, &result));

  DWORD session_id = 0;
  ::ProcessIdToSessionId(::GetCurrentProcessId(), &session_id);
  SetInt(env, result, "sessionId", session_id);
  SetInt(env, result, "activeConsoleSessionId",
         ::WTSGetActiveConsoleSessionId());

  // The desktop that currently receives input. "Default" is what we want;
  // "Winlogon" means locked / UAC secure desktop; failure usually means a
  // service or otherwise non-interactive window station.
  HDESK input_desktop = ::OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
  if (input_desktop) {
    SetString(env, result, "inputDesktop", ObjectName(input_desktop));
    ::CloseDesktop(input_desktop);
  } else {
    SetString(env, result, "inputDesktop", "");
    SetInt(env, result, "inputDesktopError", ::GetLastError());
  }
  SetString(env, result, "threadDesktop",
            ObjectName(::GetThreadDesktop(::GetCurrentThreadId())));

  HWINSTA station = ::GetProcessWindowStation();
  SetString(env, result, "windowStation", ObjectName(station));
  USEROBJECTFLAGS flags = {};
  if (::GetUserObjectInformationW(station, UOI_FLAGS, &flags, sizeof(flags),
                                  nullptr)) {
    SetBool(env, result, "windowStationVisible",
            (flags.dwFlags & WSF_VISIBLE) != 0);
  }

  // Integrity level RID: 0x2000 medium, 0x3000 high, 0x4000 system.
  HANDLE token = nullptr;
  if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)) {
    BYTE buffer[64] = {};
    DWORD size = 0;
    if (::GetTokenInformation(token, TokenIntegrityLevel, buffer,
                              sizeof(buffer), &size)) {
      auto* label = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(buffer);
      PSID sid = label->Label.Sid;
      DWORD rid = *::GetSidSubAuthority(
          sid, static_cast<DWORD>(*::GetSidSubAuthorityCount(sid) - 1));
      SetInt(env, result, "integrityLevel", rid);
    }
    ::CloseHandle(token);
  }

  SetInt(env, result, "mousePresent", ::GetSystemMetrics(SM_MOUSEPRESENT));
  SetInt(env, result, "mouseButtons", ::GetSystemMetrics(SM_CMOUSEBUTTONS));
  SetInt(env, result, "remoteSession", ::GetSystemMetrics(SM_REMOTESESSION));

  napi_value virtual_screen;
  napi_create_object(env, &virtual_screen);
  SetInt(env, virtual_screen, "x", ::GetSystemMetrics(SM_XVIRTUALSCREEN));
  SetInt(env, virtual_screen, "y", ::GetSystemMetrics(SM_YVIRTUALSCREEN));
  SetInt(env, virtual_screen, "width", ::GetSystemMetrics(SM_CXVIRTUALSCREEN));
  SetInt(env, virtual_screen, "height", ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
  napi_set_named_property(env, result, "virtualScreen", virtual_screen);

  POINT cursor = {};
  if (::GetCursorPos(&cursor)) {
    napi_value cursor_value;
    napi_create_object(env, &cursor_value);
    SetInt(env, cursor_value, "x", cursor.x);
    SetInt(env, cursor_value, "y", cursor.y);
    napi_set_named_property(env, result, "cursor", cursor_value);
  } else {
    SetInt(env, result, "cursorError", ::GetLastError());
  }

  CURSORINFO cursor_info = {};
  cursor_info.cbSize = sizeof(cursor_info);
  if (::GetCursorInfo(&cursor_info)) {
    // CURSOR_SUPPRESSED (2) is reported on touch / no-mouse machines.
    SetInt(env, result, "cursorFlags", cursor_info.flags);
  }

  // A zero-distance relative move: harmless, but tells us whether SendInput is
  // accepted at all from this process.
  INPUT probe = MouseInput(MOUSEEVENTF_MOVE);
  ::SetLastError(0);
  UINT sent = ::SendInput(1, &probe, sizeof(INPUT));
  SetInt(env, result, "sendInputProbeSent", sent);
  SetInt(env, result, "sendInputProbeError", sent == 1 ? 0 : ::GetLastError());

  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
      {"move", nullptr, Move, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"button", nullptr, Button, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"wheel", nullptr, Wheel, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"getCursorPos", nullptr, GetCursorPosition, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"isWindowAtPoint", nullptr, IsWindowAtPoint, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"describeWindowAtPoint", nullptr, DescribeWindowAtPoint, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"getDiagnostics", nullptr, GetDiagnostics, nullptr, nullptr, nullptr,
       napi_default, nullptr},
  };
  napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
