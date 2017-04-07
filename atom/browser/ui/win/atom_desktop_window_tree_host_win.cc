// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/atom_desktop_window_tree_host_win.h"

#include "atom/browser/ui/win/message_handler_delegate.h"
#include "ui/display/win/screen_win.h"

namespace atom {

AtomDesktopWindowTreeHostWin::AtomDesktopWindowTreeHostWin(
    MessageHandlerDelegate* delegate,
    views::internal::NativeWidgetDelegate* native_widget_delegate,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
        : views::DesktopWindowTreeHostWin(native_widget_delegate,
                                          desktop_native_widget_aura),
          delegate_(delegate) {
}

AtomDesktopWindowTreeHostWin::~AtomDesktopWindowTreeHostWin() {
}

bool AtomDesktopWindowTreeHostWin::PreHandleMSG(
    UINT message, WPARAM w_param, LPARAM l_param, LRESULT* result) {
  return delegate_->PreHandleMSG(message, w_param, l_param, result);
}

bool AtomDesktopWindowTreeHostWin::GetClientAreaInsets(
    gfx::Insets* insets) const {
  const auto has_frame = HasFrame();
  if (IsMaximized() && !has_frame) {
    // Maximized windows are actually bigger than the screen, see:
    // https://blogs.msdn.microsoft.com/oldnewthing/20120326-00/?p=8003
    //
    // We inset by the non-client area size to avoid cutting of the client
    // area. Despite this, there is still some overflow (1-3px) depending
    // on the DPI and side.
    const auto style = GetWindowLong(GetHWND(), GWL_STYLE) & ~WS_CAPTION;
    const auto ex_style = GetWindowLong(GetHWND(), GWL_EXSTYLE);
    RECT r = {};
    AdjustWindowRectEx(&r, style, FALSE, ex_style);

    *insets = gfx::Insets(abs(r.top), abs(r.left), abs(r.bottom) - 2, abs(r.right));
    return true;
  }

  return !has_frame;
}

}  // namespace atom
