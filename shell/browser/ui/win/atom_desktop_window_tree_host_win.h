// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WIN_ATOM_DESKTOP_WINDOW_TREE_HOST_WIN_H_
#define ATOM_BROWSER_UI_WIN_ATOM_DESKTOP_WINDOW_TREE_HOST_WIN_H_

#include <windows.h>

#include "atom/browser/native_window_views.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_win.h"

namespace atom {

class AtomDesktopWindowTreeHostWin : public views::DesktopWindowTreeHostWin {
 public:
  AtomDesktopWindowTreeHostWin(
      NativeWindowViews* native_window_view,
      views::DesktopNativeWidgetAura* desktop_native_widget_aura);
  ~AtomDesktopWindowTreeHostWin() override;

 protected:
  bool PreHandleMSG(UINT message,
                    WPARAM w_param,
                    LPARAM l_param,
                    LRESULT* result) override;
  bool HasNativeFrame() const override;

 private:
  NativeWindowViews* native_window_view_;  // weak ref

  DISALLOW_COPY_AND_ASSIGN(AtomDesktopWindowTreeHostWin);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WIN_ATOM_DESKTOP_WINDOW_TREE_HOST_WIN_H_
