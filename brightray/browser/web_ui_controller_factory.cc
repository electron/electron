// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/web_ui_controller_factory.h"

#include "browser/browser_context.h"
#include "browser/devtools_ui.h"

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"

namespace brightray {

namespace {

const char kChromeUIDevToolsBundledHost[] = "devtools";

}

WebUIControllerFactory::WebUIControllerFactory(BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DCHECK(browser_context_);
}

WebUIControllerFactory::~WebUIControllerFactory() {
}

content::WebUI::TypeID WebUIControllerFactory::GetWebUIType(
      content::BrowserContext* browser_context, const GURL& url) const {
  if (url.host() == kChromeUIDevToolsBundledHost) {
    return const_cast<WebUIControllerFactory*>(this);
  }

  return content::WebUI::kNoWebUI;
}

bool WebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context, const GURL& url) const {
  return GetWebUIType(browser_context, url) != content::WebUI::kNoWebUI;
}

bool WebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context, const GURL& url) const {
  return UseWebUIForURL(browser_context, url);
}

content::WebUIController* WebUIControllerFactory::CreateWebUIControllerForURL(
    content::WebUI* web_ui, const GURL& url) const {
  DCHECK(browser_context_);

  if (url.host() == kChromeUIDevToolsBundledHost)
    return new DevToolsUI(browser_context_, web_ui);

  return NULL;
}

}  // namespace brightray
