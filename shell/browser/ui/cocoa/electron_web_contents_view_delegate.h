// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_view_delegate.h"

#if defined(__OBJC__)
#include "content/public/browser/render_widget_host_view_mac_delegate.h"
#endif  // defined(__OBJC__)

namespace electron {

class ElectronWebContentsViewDelegate
    : public content::WebContentsViewDelegate {
 public:
  ElectronWebContentsViewDelegate();
  ~ElectronWebContentsViewDelegate() override;

  // WebContentsViewDelegate:
#if defined(__OBJC__)
  NSObject<RenderWidgetHostViewMacDelegate>* CreateRenderWidgetHostViewDelegate(
      content::RenderWidgetHost* render_widget_host,
      bool is_popup) override;
#else
  void* CreateRenderWidgetHostViewDelegate(
      content::RenderWidgetHost* render_widget_host,
      bool is_popup) override;
#endif  // defined(__OBJC__)

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronWebContentsViewDelegate);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_H_
