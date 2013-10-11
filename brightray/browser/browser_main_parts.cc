// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/browser_main_parts.h"

#include "browser/browser_context.h"
#include "browser/web_ui_controller_factory.h"

namespace brightray {

BrowserMainParts::BrowserMainParts() {
}

BrowserMainParts::~BrowserMainParts() {
}

void BrowserMainParts::PreMainMessageLoopRun() {
  browser_context_.reset(CreateBrowserContext());
  browser_context_->Initialize();

  web_ui_controller_factory_.reset(new WebUIControllerFactory(browser_context_.get()));
  content::WebUIControllerFactory::RegisterFactory(web_ui_controller_factory_.get());
}

BrowserContext* BrowserMainParts::CreateBrowserContext() {
  return new BrowserContext;
}

}
