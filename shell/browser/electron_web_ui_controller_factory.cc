// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_web_ui_controller_factory.h"

#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "electron/buildflags/buildflags.h"
#include "shell/browser/ui/devtools_ui.h"
#include "shell/browser/ui/webui/accessibility_ui.h"

namespace electron {

// static
ElectronWebUIControllerFactory* ElectronWebUIControllerFactory::GetInstance() {
  return base::Singleton<ElectronWebUIControllerFactory>::get();
}

ElectronWebUIControllerFactory::ElectronWebUIControllerFactory() = default;

ElectronWebUIControllerFactory::~ElectronWebUIControllerFactory() = default;

content::WebUI::TypeID ElectronWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (url.host() == chrome::kChromeUIDevToolsHost ||
      url.host() == chrome::kChromeUIAccessibilityHost) {
    return const_cast<ElectronWebUIControllerFactory*>(this);
  }

  return content::WebUI::kNoWebUI;
}

bool ElectronWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return GetWebUIType(browser_context, url) != content::WebUI::kNoWebUI;
}

std::unique_ptr<content::WebUIController>
ElectronWebUIControllerFactory::CreateWebUIControllerForURL(
    content::WebUI* web_ui,
    const GURL& url) {
  if (url.host() == chrome::kChromeUIDevToolsHost) {
    auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
    return std::make_unique<DevToolsUI>(browser_context, web_ui);
  } else if (url.host() == chrome::kChromeUIAccessibilityHost) {
    return std::make_unique<ElectronAccessibilityUI>(web_ui);
  }

  return std::unique_ptr<content::WebUIController>();
}

}  // namespace electron
