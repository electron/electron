// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/atom_desktop_native_widget_aura.h"

namespace atom {

AtomDesktopNativeWidgetAura::AtomDesktopNativeWidgetAura(
    views::internal::NativeWidgetDelegate* delegate)
    : views::DesktopNativeWidgetAura(delegate) {
}

void AtomDesktopNativeWidgetAura::Activate() {
  // Activate can cause the focused window to be blurred so only
  // call when the window being activated is visible. This prevents
  // hidden windows from blurring the focused window when created.
  if (IsVisible())
    views::DesktopNativeWidgetAura::Activate();
}

}  // namespace atom
