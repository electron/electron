// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/web_ui_controller_factory.h"

#include "base/memory/singleton.h"
#include "brightray/browser/devtools_ui.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"

namespace brightray {

namespace {

const char kChromeUIDevToolsBundledHost[] = "devtools";

}  // namespace

// static
WebUIControllerFactory* WebUIControllerFactory::GetInstance() {
  return base::Singleton<WebUIControllerFactory>::get();
}

WebUIControllerFactory::WebUIControllerFactory() {}

WebUIControllerFactory::~WebUIControllerFactory() {}

content::WebUI::TypeID WebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  if (url.host() == kChromeUIDevToolsBundledHost) {
    return const_cast<WebUIControllerFactory*>(this);
  }

  return content::WebUI::kNoWebUI;
}

bool WebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return GetWebUIType(browser_context, url) != content::WebUI::kNoWebUI;
}

bool WebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return UseWebUIForURL(browser_context, url);
}

std::unique_ptr<content::WebUIController>
WebUIControllerFactory::CreateWebUIControllerForURL(content::WebUI* web_ui,
                                                    const GURL& url) const {
  if (url.host() == kChromeUIDevToolsBundledHost) {
    auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
    return std::make_unique<DevToolsUI>(browser_context, web_ui);
  }
  return std::unique_ptr<content::WebUIController>();
}

}  // namespace brightray
