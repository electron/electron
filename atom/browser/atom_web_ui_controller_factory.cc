// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_web_ui_controller_factory.h"

#include <string>

#include "atom/browser/ui/webui/pdf_viewer_ui.h"
#include "atom/common/atom_constants.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "content/public/browser/web_contents.h"

namespace atom {

// static
AtomWebUIControllerFactory* AtomWebUIControllerFactory::GetInstance() {
  return base::Singleton<AtomWebUIControllerFactory>::get();
}

AtomWebUIControllerFactory::AtomWebUIControllerFactory() {}

AtomWebUIControllerFactory::~AtomWebUIControllerFactory() {}

content::WebUI::TypeID AtomWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  if (url.host() == kPdfViewerUIHost) {
    return const_cast<AtomWebUIControllerFactory*>(this);
  }

  return content::WebUI::kNoWebUI;
}

bool AtomWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return GetWebUIType(browser_context, url) != content::WebUI::kNoWebUI;
}

bool AtomWebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return UseWebUIForURL(browser_context, url);
}

content::WebUIController*
AtomWebUIControllerFactory::CreateWebUIControllerForURL(content::WebUI* web_ui,
                                                        const GURL& url) const {
  if (url.host() == kPdfViewerUIHost) {
    base::StringPairs toplevel_params;
    base::SplitStringIntoKeyValuePairs(url.query(), '=', '&', &toplevel_params);
    std::string stream_id, src;
    for (const auto& param : toplevel_params) {
      if (param.first == kPdfPluginSrc) {
        src = param.second;
      }
    }
    auto browser_context = web_ui->GetWebContents()->GetBrowserContext();
    return new PdfViewerUI(browser_context, web_ui, src);
  }
  return nullptr;
}

}  // namespace atom
