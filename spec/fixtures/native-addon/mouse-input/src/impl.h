// Platform backends for the macOS and Linux (X11) builds of the addon. The
// Windows build is self contained in main_win.cc.
//
// Coordinates are global screen coordinates as the platform's input APIs use
// them: points (DIPs) with the origin at the top left of the main display on
// macOS, root window pixels on X11.

#ifndef MOUSE_INPUT_SRC_IMPL_H_
#define MOUSE_INPUT_SRC_IMPL_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace mouse_input {

enum class Button { kLeft, kRight, kMiddle };

// Mirrors what SendInput() reports on Windows so lib/index.js can check all
// platforms the same way: |sent| out of |expected| events were injected,
// |error| is a platform specific code when they were not.
struct InjectResult {
  int sent = 0;
  int expected = 0;
  int64_t error = 0;
};

// These may block briefly and are called on a libuv worker thread.
InjectResult Move(int x, int y);
InjectResult Press(Button button, bool down);
// |delta| is in Windows WHEEL_DELTA (120) units per notch; positive scrolls
// up (vertical) or right (horizontal).
InjectResult Wheel(int delta, bool horizontal);

// Called on the JS thread.
bool GetCursorPos(int* x, int* y, int64_t* error);

#if defined(__APPLE__)
// Posts a left click to process |pid| without going through the window server.
// (x, y) is in the coordinates of its window |window_number|, whose origin is
// the bottom left corner. Called on the JS thread.
InjectResult PostClickToWindow(int pid, int window_number, int x, int y);
#endif

// Whether the window identified by |handle| (the bytes of
// BrowserWindow.getNativeWindowHandle()) is what the OS hit tests at the
// screen point. Returns false and sets |supported| to false where there is no
// reliable way to ask.
bool IsWindowAtPoint(const void* handle,
                     size_t handle_size,
                     int x,
                     int y,
                     bool* supported);

// Key/value pairs describing whether input injection can work on this
// machine. Values are strings, integers or booleans.
struct DiagnosticValue {
  enum class Type { kString, kInt, kBool };
  Type type = Type::kString;
  std::string string_value;
  int64_t int_value = 0;
  bool bool_value = false;
};
using Diagnostics = std::vector<std::pair<std::string, DiagnosticValue>>;
Diagnostics GetDiagnostics();

inline DiagnosticValue StringValue(std::string value) {
  DiagnosticValue v;
  v.type = DiagnosticValue::Type::kString;
  v.string_value = std::move(value);
  return v;
}
inline DiagnosticValue IntValue(int64_t value) {
  DiagnosticValue v;
  v.type = DiagnosticValue::Type::kInt;
  v.int_value = value;
  return v;
}
inline DiagnosticValue BoolValue(bool value) {
  DiagnosticValue v;
  v.type = DiagnosticValue::Type::kBool;
  v.bool_value = value;
  return v;
}

}  // namespace mouse_input

#endif  // MOUSE_INPUT_SRC_IMPL_H_
