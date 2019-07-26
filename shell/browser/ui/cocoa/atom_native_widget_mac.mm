// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/atom_native_widget_mac.h"

#include "shell/browser/ui/cocoa/atom_ns_window.h"

namespace electron {

AtomNativeWidgetMac::AtomNativeWidgetMac(
    NativeWindowMac* shell,
    NSUInteger style_mask,
    views::internal::NativeWidgetDelegate* delegate)
    : views::NativeWidgetMac(delegate),
      shell_(shell),
      style_mask_(style_mask) {}

AtomNativeWidgetMac::~AtomNativeWidgetMac() {}

NativeWidgetMacNSWindow* AtomNativeWidgetMac::CreateNSWindow(
    const remote_cocoa::mojom::CreateWindowParams* params) {
  return [[[AtomNSWindow alloc] initWithShell:shell_
                                    styleMask:style_mask_] autorelease];
}

}  // namespace electron
