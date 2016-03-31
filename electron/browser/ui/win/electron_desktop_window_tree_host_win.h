// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_
#define ELECTRON_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_

#include <windows.h>

#include <vector>

#include "electron/browser/native_window.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_win.h"

namespace electron {

class MessageHandlerDelegate;

class ElectronDesktopWindowTreeHostWin : public views::DesktopWindowTreeHostWin {
 public:
  ElectronDesktopWindowTreeHostWin(
      MessageHandlerDelegate* delegate,
      views::internal::NativeWidgetDelegate* native_widget_delegate,
      views::DesktopNativeWidgetAura* desktop_native_widget_aura);
  ~ElectronDesktopWindowTreeHostWin() override;

 protected:
  bool PreHandleMSG(
      UINT message, WPARAM w_param, LPARAM l_param, LRESULT* result) override;

 private:
  MessageHandlerDelegate* delegate_;  // weak ref

  DISALLOW_COPY_AND_ASSIGN(ElectronDesktopWindowTreeHostWin);
};

}  // namespace electron

#endif  // ELECTRON_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_
