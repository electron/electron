// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_
#define SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_

#include <windows.h>

#include "shell/browser/native_window_views.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_win.h"

namespace electron {

class ElectronDesktopWindowTreeHostWin
    : public views::DesktopWindowTreeHostWin {
 public:
  ElectronDesktopWindowTreeHostWin(
      NativeWindowViews* native_window_view,
      views::DesktopNativeWidgetAura* desktop_native_widget_aura);
  ~ElectronDesktopWindowTreeHostWin() override;

 protected:
  bool PreHandleMSG(UINT message,
                    WPARAM w_param,
                    LPARAM l_param,
                    LRESULT* result) override;
  bool ShouldPaintAsActive() const override;
  bool HasNativeFrame() const override;
  bool GetDwmFrameInsetsInPixels(gfx::Insets* insets) const override;
  bool GetClientAreaInsets(gfx::Insets* insets,
                           HMONITOR monitor) const override;
  bool HandleMouseEvent(ui::MouseEvent* event) override;

 private:
  NativeWindowViews* native_window_view_;  // weak ref

  DISALLOW_COPY_AND_ASSIGN(ElectronDesktopWindowTreeHostWin);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_
