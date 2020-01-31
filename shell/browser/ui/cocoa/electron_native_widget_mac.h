// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_

#include "ui/views/widget/native_widget_mac.h"

namespace electron {

class NativeWindowMac;

class ElectronNativeWidgetMac : public views::NativeWidgetMac {
 public:
  ElectronNativeWidgetMac(NativeWindowMac* shell,
                          NSUInteger style_mask,
                          views::internal::NativeWidgetDelegate* delegate);
  ~ElectronNativeWidgetMac() override;

 protected:
  // NativeWidgetMac:
  NativeWidgetMacNSWindow* CreateNSWindow(
      const remote_cocoa::mojom::CreateWindowParams* params) override;

 private:
  NativeWindowMac* shell_;
  NSUInteger style_mask_;

  DISALLOW_COPY_AND_ASSIGN(ElectronNativeWidgetMac);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_
