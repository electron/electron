// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_web_ui_controller_factory.h"

#include <string>

#include "content/public/browser/web_contents.h"
#include "electron/buildflags/buildflags.h"
#include "shell/browser/ui/devtools_ui.h"

namespace electron {

namespace {

const char kChromeUIDevToolsBundledHost[] = "devtools";

}  // namespace

// static
AtomWebUIControllerFactory* AtomWebUIControllerFactory::GetInstance() {
  return base::Singleton<AtomWebUIControllerFactory>::get();
}

AtomWebUIControllerFactory::AtomWebUIControllerFactory() = default;

AtomWebUIControllerFactory::~AtomWebUIControllerFactory() = default;

content::WebUI::TypeID AtomWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (url.host() == kChromeUIDevToolsBundledHost) {
    return const_cast<AtomWebUIControllerFactory*>(this);
  }

  return content::WebUI::kNoWebUI;
}

bool AtomWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return GetWebUIType(browser_context, url) != content::WebUI::kNoWebUI;
}

bool AtomWebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return UseWebUIForURL(browser_context, url);
}

std::unique_ptr<content::WebUIController>
AtomWebUIControllerFactory::CreateWebUIControllerForURL(content::WebUI* web_ui,
                                                        const GURL& url) {
  if (url.host() == kChromeUIDevToolsBundledHost) {
    auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
    return std::make_unique<DevToolsUI>(browser_context, web_ui);
  }
  return std::unique_ptr<content::WebUIController>();
}

}  // namespace electron
