// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/browser_main_parts.h"

#include "browser/browser_context.h"
#include "browser/devtools_delegate.h"

#include "content/public/browser/devtools_http_handler.h"
#include "net/base/tcp_listen_socket.h"

namespace brightray {

BrowserMainParts::BrowserMainParts() {
}

BrowserMainParts::~BrowserMainParts() {
  devtools_http_handler_->Stop();
  devtools_http_handler_ = nullptr;
}

void BrowserMainParts::PreMainMessageLoopRun() {
  browser_context_.reset(CreateBrowserContext());

  // These two objects are owned by devtools_http_handler_.
  auto delegate = new DevToolsDelegate;
  auto factory = new net::TCPListenSocketFactory("127.0.0.1", 0);

  devtools_http_handler_ = content::DevToolsHttpHandler::Start(factory, std::string(), delegate);
}

BrowserContext* BrowserMainParts::CreateBrowserContext() {
  return new BrowserContext;
}

}
