// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tab_contents/chrome_web_contents_view_delegate_views.h"

#include <memory>
#include <utility>

#include "chrome/browser/defaults.h"
#include "chrome/browser/ui/aura/tab_contents/web_drag_bookmark_handler_aura.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/sad_tab_helper.h"
#include "chrome/browser/ui/tab_contents/chrome_web_contents_menu_helper.h"
#include "chrome/browser/ui/tab_contents/chrome_web_contents_view_handle_drop.h"
#include "chrome/browser/ui/views/renderer_context_menu/render_view_context_menu_views.h"
#include "chrome/browser/ui/views/sad_tab_view.h"
#include "chrome/browser/ui/views/tab_contents/chrome_web_contents_view_focus_helper.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/drop_data.h"
#include "ui/views/widget/widget.h"

ElectronWebContentsViewDelegateViews::ElectronWebContentsViewDelegateViews(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

ElectronWebContentsViewDelegateViews::~ElectronWebContentsViewDelegateViews() =
    default;

static std::unique_ptr<content::WebContentsViewDelegate> CreateWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return std::make_unique<ElectronWebContentsViewDelegateViews>(web_contents);
}
