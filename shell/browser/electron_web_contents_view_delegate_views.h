// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_VIEWS_H_
#define ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_VIEWS_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "components/renderer_context_menu/context_menu_delegate.h"
#include "content/public/browser/web_contents_view_delegate.h"

class RenderViewContextMenuBase;
class ChromeWebContentsViewFocusHelper;

namespace content {
class WebContents;
class WebDragDestDelegate;
class RenderFrameHost;
}  // namespace content

// A chrome specific class that extends WebContentsViewWin with features like
// focus management, which live in chrome.
class ElectronWebContentsViewDelegateViews
    : public content::WebContentsViewDelegate {
 public:
  explicit ElectronWebContentsViewDelegateViews(
      content::WebContents* web_contents);

  ElectronWebContentsViewDelegateViews(
      const ElectronWebContentsViewDelegateViews&) = delete;
  ElectronWebContentsViewDelegateViews& operator=(
      const ElectronWebContentsViewDelegateViews&) = delete;

  ~ElectronWebContentsViewDelegateViews() override;
 private:

  raw_ptr<content::WebContents> web_contents_;

};

#endif  // CHROME_BROWSER_UI_VIEWS_TAB_CONTENTS_CHROME_WEB_CONTENTS_VIEW_DELEGATE_VIEWS_H_
