// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/electron_web_contents_view_delegate.h"

#include "shell/browser/ui/cocoa/electron_render_widget_host_view_mac_delegate.h"

namespace electron {

ElectronWebContentsViewDelegate::ElectronWebContentsViewDelegate() {}

ElectronWebContentsViewDelegate::~ElectronWebContentsViewDelegate() {}

NSObject<RenderWidgetHostViewMacDelegate>*
ElectronWebContentsViewDelegate::CreateRenderWidgetHostViewDelegate(
    content::RenderWidgetHost* render_widget_host,
    bool is_popup) {
  return [[ElectronRenderWidgetHostViewMacDelegate alloc]
      initWithRenderWidgetHost:render_widget_host];
}

}  // namespace electron
