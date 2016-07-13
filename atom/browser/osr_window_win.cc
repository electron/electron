// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <windows.h>

#include "atom/browser/osr_window.h"

namespace atom {

class DummyWindowWin : public gfx::WindowImpl {
public:
  DummyWindowWin() {
    // Create a hidden 1x1 borderless window.
    set_window_style(WS_POPUP | WS_SYSMENU);
    Init(NULL, gfx::Rect(0, 0, 1, 1));
  }

  ~DummyWindowWin() override {
    DestroyWindow(hwnd());
  }

private:
  CR_BEGIN_MSG_MAP_EX(DummyWindowWin)
    CR_MSG_WM_PAINT(OnPaint)
  CR_END_MSG_MAP()

  void OnPaint(HDC dc) {
    ValidateRect(hwnd(), NULL);
  }

  DISALLOW_COPY_AND_ASSIGN(DummyWindowWin);
};

void OffScreenWindow::CreatePlatformWidget() {
  DCHECK(!window_);
  window_.reset(new DummyWindowWin());
  compositor_widget_ = window_->hwnd();
}

}  // namespace atom
