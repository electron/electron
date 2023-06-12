// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/widget/native_widget_mac.h"

namespace electron {

class NativeWindowMac;

class ElectronNativeWidgetMac : public views::NativeWidgetMac {
 public:
  ElectronNativeWidgetMac(NativeWindowMac* shell,
                          const std::string& window_type,
                          NSUInteger style_mask,
                          views::internal::NativeWidgetDelegate* delegate);
  ~ElectronNativeWidgetMac() override;

  // disable copy
  ElectronNativeWidgetMac(const ElectronNativeWidgetMac&) = delete;
  ElectronNativeWidgetMac& operator=(const ElectronNativeWidgetMac&) = delete;

 protected:
  // NativeWidgetMac:
  NativeWidgetMacNSWindow* CreateNSWindow(
      const remote_cocoa::mojom::CreateWindowParams* params) override;

 private:
  raw_ptr<NativeWindowMac> shell_;
  std::string window_type_;
  NSUInteger style_mask_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_
