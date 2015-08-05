// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WIN_ATOM_DESKTOP_WINDOW_TREE_HOST_WIN_H_
#define ATOM_BROWSER_UI_WIN_ATOM_DESKTOP_WINDOW_TREE_HOST_WIN_H_

#include <windows.h>

#include <vector>

#include "atom/browser/native_window.h"
#include "atom/browser/ui/win/thumbar_host.h"
#include "base/memory/scoped_ptr.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_win.h"

namespace atom {

class AtomDesktopWindowTreeHostWin : public views::DesktopWindowTreeHostWin {
 public:
  AtomDesktopWindowTreeHostWin(
      views::internal::NativeWidgetDelegate* native_widget_delegate,
      views::DesktopNativeWidgetAura* desktop_native_widget_aura);
  ~AtomDesktopWindowTreeHostWin();

  bool SetThumbarButtons(
      HWND window,
      const std::vector<NativeWindow::ThumbarButton>& buttons);

 protected:
  bool PreHandleMSG(UINT message,
                    WPARAM w_param,
                    LPARAM l_param,
                    LRESULT* result) override;

 private:
  scoped_ptr<ThumbarHost> thumbar_host_;
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WIN_ATOM_DESKTOP_WINDOW_TREE_HOST_WIN_H_
