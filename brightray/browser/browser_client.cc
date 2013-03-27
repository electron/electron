// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser_client.h"

#include "browser_context.h"
#include "browser_main_parts.h"

namespace brightray {

BrowserClient::BrowserClient() {
}

BrowserClient::~BrowserClient() {
}

BrowserContext* BrowserClient::browser_context() {
  return browser_main_parts_->browser_context();
}

BrowserMainParts* BrowserClient::OverrideCreateBrowserMainParts(const content::MainFunctionParams&) {
  return new BrowserMainParts;
}

content::BrowserMainParts* BrowserClient::CreateBrowserMainParts(const content::MainFunctionParams& parameters) {
  DCHECK(!browser_main_parts_);
  browser_main_parts_ = OverrideCreateBrowserMainParts(parameters);
  return browser_main_parts_;
}

net::URLRequestContextGetter* BrowserClient::CreateRequestContext(content::BrowserContext* browser_context, content::ProtocolHandlerMap* protocol_handlers) {
  return static_cast<BrowserContext*>(browser_context)->CreateRequestContext(protocol_handlers);
}

}
