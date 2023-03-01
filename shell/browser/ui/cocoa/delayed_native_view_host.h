// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_DELAYED_NATIVE_VIEW_HOST_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_DELAYED_NATIVE_VIEW_HOST_H_

#include "ui/views/controls/native/native_view_host.h"

namespace electron {

// Automatically attach the native view after the NativeViewHost is attached to
// a widget. (Attaching it directly would cause crash.)
class DelayedNativeViewHost : public views::NativeViewHost {
 public:
  explicit DelayedNativeViewHost(gfx::NativeView native_view);
  ~DelayedNativeViewHost() override;

  // disable copy
  DelayedNativeViewHost(const DelayedNativeViewHost&) = delete;
  DelayedNativeViewHost& operator=(const DelayedNativeViewHost&) = delete;

  // views::View:
  void ViewHierarchyChanged(
      const views::ViewHierarchyChangedDetails& details) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

 private:
  gfx::NativeView native_view_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_DELAYED_NATIVE_VIEW_HOST_H_
