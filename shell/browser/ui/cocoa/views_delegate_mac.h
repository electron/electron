// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_

#include "ui/views/views_delegate.h"

namespace electron {

class ViewsDelegateMac : public views::ViewsDelegate {
 public:
  ViewsDelegateMac();
  ~ViewsDelegateMac() override;

  // disable copy
  ViewsDelegateMac(const ViewsDelegateMac&) = delete;
  ViewsDelegateMac& operator=(const ViewsDelegateMac&) = delete;

  // ViewsDelegate:
  void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) override;
  ui::ContextFactory* GetContextFactory() override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_
