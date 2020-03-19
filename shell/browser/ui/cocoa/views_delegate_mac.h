// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_
#define SHELL_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_

#include "ui/views/views_delegate.h"

namespace electron {

class ViewsDelegateMac : public views::ViewsDelegate {
 public:
  ViewsDelegateMac();
  ~ViewsDelegateMac() override;

  // ViewsDelegate:
  void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) override;
  ui::ContextFactory* GetContextFactory() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ViewsDelegateMac);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_
