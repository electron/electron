// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/ui/devtools_ui.h"

#include <memory>

#include "content/public/browser/web_ui.h"
#include "shell/browser/ui/devtools_ui_bundle_data_source.h"
#include "shell/browser/ui/devtools_ui_theme_data_source.h"

namespace electron {

DevToolsUI::DevToolsUI(content::BrowserContext* browser_context,
                       content::WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->SetBindings(content::BindingsPolicySet());
  content::URLDataSource::Add(browser_context,
                              std::make_unique<BundledDataSource>());
  content::URLDataSource::Add(browser_context,
                              std::make_unique<ThemeDataSource>());
}

}  // namespace electron
