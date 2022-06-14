// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ROOT_VIEW_MAC_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ROOT_VIEW_MAC_H_

#include "ui/views/view.h"

namespace electron {

class NativeWindow;

class RootViewMac : public views::View {
 public:
  explicit RootViewMac(NativeWindow* window);
  ~RootViewMac() override;

  // disable copy
  RootViewMac(const RootViewMac&) = delete;
  RootViewMac& operator=(const RootViewMac&) = delete;

  // views::View:
  void Layout() override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;

 private:
  // Parent window, weak ref.
  NativeWindow* window_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ROOT_VIEW_MAC_H_
