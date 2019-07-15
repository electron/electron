// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_web_ui_controller_factory.h"

#include <string>

#include "electron/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/escape.h"
#include "shell/browser/ui/webui/pdf_viewer_ui.h"
#include "shell/common/atom_constants.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

#include "content/public/browser/web_contents.h"
#include "shell/browser/ui/devtools_ui.h"

namespace electron {

namespace {

const char kChromeUIDevToolsBundledHost[] = "devtools";

}  // namespace

// static
AtomWebUIControllerFactory* AtomWebUIControllerFactory::GetInstance() {
  return base::Singleton<AtomWebUIControllerFactory>::get();
}

AtomWebUIControllerFactory::AtomWebUIControllerFactory() {}

AtomWebUIControllerFactory::~AtomWebUIControllerFactory() {}

content::WebUI::TypeID AtomWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  if (url.host() == kPdfViewerUIHost) {
    return const_cast<AtomWebUIControllerFactory*>(this);
  }
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)
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
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  if (url.host() == kPdfViewerUIHost) {
    base::StringPairs toplevel_params;
    base::SplitStringIntoKeyValuePairs(url.query(), '=', '&', &toplevel_params);
    std::string src;

    const net::UnescapeRule::Type unescape_rules =
        net::UnescapeRule::SPACES | net::UnescapeRule::PATH_SEPARATORS |
        net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
        net::UnescapeRule::REPLACE_PLUS_WITH_SPACE;

    for (const auto& param : toplevel_params) {
      if (param.first == kPdfPluginSrc) {
        src = net::UnescapeURLComponent(param.second, unescape_rules);
      }
    }
    if (url.has_ref()) {
      src = src + '#' + url.ref();
    }
    auto browser_context = web_ui->GetWebContents()->GetBrowserContext();
    return new PdfViewerUI(browser_context, web_ui, src);
  }
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)
  if (url.host() == kChromeUIDevToolsBundledHost) {
    auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
    return std::make_unique<DevToolsUI>(browser_context, web_ui);
  }
  return std::unique_ptr<content::WebUIController>();
}

}  // namespace electron
