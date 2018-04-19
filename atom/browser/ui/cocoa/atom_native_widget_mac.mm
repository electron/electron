// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/cocoa/atom_native_widget_mac.h"

namespace atom {

AtomNativeWidgetMac::AtomNativeWidgetMac(
    views::internal::NativeWidgetDelegate* delegate)
    : views::NativeWidgetMac(delegate) {
}

AtomNativeWidgetMac::~AtomNativeWidgetMac() {
}

NativeWidgetMacNSWindow* AtomNativeWidgetMac::CreateNSWindow(
    const views::Widget::InitParams& params) {
  return views::NativeWidgetMac::CreateNSWindow(params);
}

}  // namespace atom
