// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_web_ui_controller_factory.h"

#include <string>

#include "electron/buildflags/buildflags.h"

#include "content/public/browser/web_contents.h"
#include "shell/browser/ui/devtools_ui.h"

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "chrome/browser/ui/webui/extensions/extensions_ui.h"
#include "content/public/browser/web_ui.h"

using content::WebUI;
using content::WebUIController;
#endif

namespace electron {

namespace {

const char kChromeUIDevToolsBundledHost[] = "devtools";
const char kChromeUIExtensionsHost[] = "extensions";

#if BUILDFLAG(ENABLE_PDF_VIEWER)
// A function for creating a new WebUI. The caller owns the return value, which
// may be NULL (for example, if the URL refers to an non-existent extension).
typedef WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                 const GURL& url);

// Template for defining WebUIFactoryFunction.
template <class T>
WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui);
}
#endif

}  // namespace

// static
AtomWebUIControllerFactory* AtomWebUIControllerFactory::GetInstance() {
  return base::Singleton<AtomWebUIControllerFactory>::get();
}

AtomWebUIControllerFactory::AtomWebUIControllerFactory() {}

AtomWebUIControllerFactory::~AtomWebUIControllerFactory() {}

content::WebUI::TypeID AtomWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  if (url.host() == kChromeUIDevToolsBundledHost) {
    return const_cast<AtomWebUIControllerFactory*>(this);
  }

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  if (url.host() == kChromeUIExtensionsHost) {
    WebUIFactoryFunction function = &NewWebUI<extensions::ExtensionsUI>;
    return reinterpret_cast<WebUI::TypeID>(function);
  }
#endif

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
                                                        const GURL& url) const {
  if (url.host() == kChromeUIDevToolsBundledHost) {
    auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
    return std::make_unique<DevToolsUI>(browser_context, web_ui);
  }
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  if (url.host() == kChromeUIExtensionsHost) {
    WebUIFactoryFunction function = &NewWebUI<extensions::ExtensionsUI>;
    return base::WrapUnique((*function)(web_ui, url));
  }
#endif
  return std::unique_ptr<content::WebUIController>();
}

}  // namespace electron
