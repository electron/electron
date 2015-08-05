// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/atom_desktop_window_tree_host_win.h"

#include <shobjidl.h>

#include "atom/browser/ui/win/thumbar_host.h"

namespace atom {

AtomDesktopWindowTreeHostWin::AtomDesktopWindowTreeHostWin(
    views::internal::NativeWidgetDelegate* native_widget_delegate,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
        : views::DesktopWindowTreeHostWin(native_widget_delegate,
                                          desktop_native_widget_aura) {
}

AtomDesktopWindowTreeHostWin::~AtomDesktopWindowTreeHostWin() {
}

bool AtomDesktopWindowTreeHostWin::SetThumbarButtons(
    HWND window,
    const std::vector<ThumbarHost::ThumbarButton>& buttons) {
  if (!thumbar_host_.get()) {
    thumbar_host_.reset(new ThumbarHost(window));
  }
  return thumbar_host_->SetThumbarButtons(buttons);
}

bool AtomDesktopWindowTreeHostWin::PreHandleMSG(UINT message,
                                                WPARAM w_param,
                                                LPARAM l_param,
                                                LRESULT* result) {
  switch (message) {
    case WM_COMMAND: {
       // Handle thumbar button click message.
       int id = LOWORD(w_param);
       int thbn_message = HIWORD(w_param);
       if (thbn_message == THBN_CLICKED && thumbar_host_ &&
           thumbar_host_->HandleThumbarButtonEvent(id))
         return true;
    }
  }

  return false;
}

}  // namespace atom
