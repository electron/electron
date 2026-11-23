// Picks the Linux backend for the session the specs run in, the same way
// spec/lib/spec-helpers.ts decides isWayland: Wayland when
// XDG_SESSION_TYPE=wayland or WAYLAND_DISPLAY is set, X11 otherwise.
// XTEST cannot reach Wayland windows even where XWayland provides it.

#include "impl_linux.h"

#include <cstdlib>
#include <cstring>

namespace mouse_input {

namespace {

bool UseWayland() {
  static const bool use_wayland = [] {
    const char* session_type = std::getenv("XDG_SESSION_TYPE");
    if (session_type && std::strcmp(session_type, "wayland") == 0)
      return true;
    const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
    return wayland_display && *wayland_display;
  }();
  return use_wayland;
}

}  // namespace

InjectResult Move(int x, int y) {
  return UseWayland() ? wayland::Move(x, y) : x11::Move(x, y);
}

InjectResult Press(Button button, bool down) {
  return UseWayland() ? wayland::Press(button, down) : x11::Press(button, down);
}

InjectResult Wheel(int delta, bool horizontal) {
  return UseWayland() ? wayland::Wheel(delta, horizontal)
                      : x11::Wheel(delta, horizontal);
}

bool GetCursorPos(int* x, int* y, int64_t* error) {
  return UseWayland() ? wayland::GetCursorPos(x, y, error)
                      : x11::GetCursorPos(x, y, error);
}

bool IsWindowAtPoint(const void* handle,
                     size_t handle_size,
                     int x,
                     int y,
                     bool* supported) {
  return UseWayland()
             ? wayland::IsWindowAtPoint(handle, handle_size, x, y, supported)
             : x11::IsWindowAtPoint(handle, handle_size, x, y, supported);
}

Diagnostics GetDiagnostics() {
  return UseWayland() ? wayland::GetDiagnostics() : x11::GetDiagnostics();
}

}  // namespace mouse_input
