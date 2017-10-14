// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/window_util.h"

namespace window_util {

void ForceFocusWindow(HWND hwnd) {
  // To unlock SetForegroundWindow we need to imitate pressing the Alt key
  // This circumvents the ForegroundLockTimeout in Windows 10
  bool pressed = false;
  if ((GetAsyncKeyState(VK_MENU) & 0x8000) == 0) {
    pressed = true;
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
  }

  SetForegroundWindow(hwnd);
  SetFocus(hwnd);

  if (pressed) {
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
  }
}

}  // namespace window_util
