// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_VIEWS_MAC_H_
#define ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_VIEWS_MAC_H_

#include "content/public/browser/web_contents_view_delegate.h"
#if BUILDFLAG(IS_MAC)
#include "shell/browser/electron_web_contents_view_delegate_views_mac.h"
#endif

namespace content {
class RenderWidgetHostView;
class WebContents;
} 

class ElectronWebContentsViewDelegateViewsMac : public content::WebContentsViewDelegate {
 public:
  ElectronWebContentsViewDelegateViewsMac(content::WebContents* web_contents);

  ElectronWebContentsViewDelegateViewsMac(
      const ElectronWebContentsViewDelegateViewsMac&) = delete;
  ElectronWebContentsViewDelegateViewsMac& operator=(
      const ElectronWebContentsViewDelegateViewsMac&) = delete;

  ~ElectronWebContentsViewDelegateViewsMac() override;
  
  NSObject<RenderWidgetHostViewMacDelegate>* GetDelegateForHost(
      content::RenderWidgetHost* render_widget_host,
      bool is_popup) override;
private:
  raw_ptr<content::WebContents> web_contents_;
};

#endif  // ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_VIEWS_MAC_H_
