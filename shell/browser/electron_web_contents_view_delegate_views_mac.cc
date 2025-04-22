// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/electron_web_contents_view_delegate_views_mac.h"

ElectronWebContentsViewDelegateViewsMac::ElectronWebContentsViewDelegateViewsMac(
content::WebContents* web_contents) : web_contents_(web_contents) {}

ElectronWebContentsViewDelegateViewsMac::~ElectronWebContentsViewDelegateViewsMac() = default;

NSObject<RenderWidgetHostViewMacDelegate>*
ElectronWebContentsViewDelegateViewsMac::GetDelegateForHost(
    content::RenderWidgetHost* render_widget_host,
    bool is_popup) {
  // We don't need a delegate for popups since they don't have
  // overscroll.
  if (is_popup) {
    return nil;
  }
  return [[ElectronRenderWidgetHostViewMacDelegate alloc]
      initWithRenderWidgetHost:render_widget_host];
  return nullptr;
}
