// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/browser/ui/win/atom_desktop_window_tree_host_win.h"

#include "electron/browser/ui/win/message_handler_delegate.h"

namespace electron {

ElectronDesktopWindowTreeHostWin::ElectronDesktopWindowTreeHostWin(
    MessageHandlerDelegate* delegate,
    views::internal::NativeWidgetDelegate* native_widget_delegate,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
        : views::DesktopWindowTreeHostWin(native_widget_delegate,
                                          desktop_native_widget_aura),
          delegate_(delegate) {
}

ElectronDesktopWindowTreeHostWin::~ElectronDesktopWindowTreeHostWin() {
}

bool ElectronDesktopWindowTreeHostWin::PreHandleMSG(
    UINT message, WPARAM w_param, LPARAM l_param, LRESULT* result) {
  return delegate_->PreHandleMSG(message, w_param, l_param, result);
}

}  // namespace electron
