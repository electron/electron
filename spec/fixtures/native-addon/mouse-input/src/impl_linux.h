// The two Linux backends behind impl.h. impl_linux.cc picks one per process.

#ifndef MOUSE_INPUT_SRC_IMPL_LINUX_H_
#define MOUSE_INPUT_SRC_IMPL_LINUX_H_

#include "impl.h"

namespace mouse_input {

#define MOUSE_INPUT_DECLARE_BACKEND                                          \
  InjectResult Move(int x, int y);                                           \
  InjectResult Press(Button button, bool down);                              \
  InjectResult Wheel(int delta, bool horizontal);                            \
  bool GetCursorPos(int* x, int* y, int64_t* error);                         \
  bool IsWindowAtPoint(const void* handle, size_t handle_size, int x, int y, \
                       bool* supported);                                     \
  Diagnostics GetDiagnostics();

// XTEST (impl_linux_x11.cc).
namespace x11 {
MOUSE_INPUT_DECLARE_BACKEND
}  // namespace x11

// Chromium's zcr_ui_controls_v2 protocol (impl_linux_wayland.cc).
namespace wayland {
MOUSE_INPUT_DECLARE_BACKEND
}  // namespace wayland

#undef MOUSE_INPUT_DECLARE_BACKEND

}  // namespace mouse_input

#endif  // MOUSE_INPUT_SRC_IMPL_LINUX_H_
