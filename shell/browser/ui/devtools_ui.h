// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_H_
#define ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_H_

#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_ui_controller.h"

namespace electron {

class DevToolsUI : public content::WebUIController {
 public:
  explicit DevToolsUI(content::BrowserContext* browser_context,
                      content::WebUI* web_ui);

  // disable copy
  DevToolsUI(const DevToolsUI&) = delete;
  DevToolsUI& operator=(const DevToolsUI&) = delete;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_H_
