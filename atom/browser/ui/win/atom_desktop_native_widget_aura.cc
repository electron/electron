// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/atom_desktop_native_widget_aura.h"

namespace atom {

AtomDeskopNativeWidgetAura::AtomDeskopNativeWidgetAura(
    views::internal::NativeWidgetDelegate* delegate,
    NativeWindowViews* window)
    : views::DesktopNativeWidgetAura(delegate),
      window_(window) {
}

bool AtomDeskopNativeWidgetAura::CanFocus() {
  return window_->CanFocus();
}

}  // namespace atom
