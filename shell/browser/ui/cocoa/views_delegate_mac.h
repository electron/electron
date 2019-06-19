// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_
#define ATOM_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_

#include "ui/views/views_delegate.h"

namespace atom {

class ViewsDelegateMac : public views::ViewsDelegate {
 public:
  ViewsDelegateMac();
  ~ViewsDelegateMac() override;

  // ViewsDelegate:
  void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) override;
  ui::ContextFactory* GetContextFactory() override;
  ui::ContextFactoryPrivate* GetContextFactoryPrivate() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ViewsDelegateMac);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_
