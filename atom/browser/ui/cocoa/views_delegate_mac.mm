// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/cocoa/views_delegate_mac.h"

#include "content/public/browser/context_factory.h"
#include "ui/views/widget/native_widget_mac.h"

namespace atom {

ViewsDelegateMac::ViewsDelegateMac() {}

ViewsDelegateMac::~ViewsDelegateMac() {}

void ViewsDelegateMac::OnBeforeWidgetInit(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  DCHECK(params->native_widget);
}

ui::ContextFactory* ViewsDelegateMac::GetContextFactory() {
  return content::GetContextFactory();
}

ui::ContextFactoryPrivate* ViewsDelegateMac::GetContextFactoryPrivate() {
  return content::GetContextFactoryPrivate();
}

}  // namespace atom
