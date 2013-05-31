// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser_client.h"

#include "browser_context.h"
#include "browser_main_parts.h"
#include "notification_presenter.h"

namespace brightray {

namespace {

BrowserClient* g_browser_client;

}

BrowserClient* BrowserClient::Get() {
  return g_browser_client;
}

BrowserClient::BrowserClient()
    : browser_main_parts_() {
  DCHECK(!g_browser_client);
  g_browser_client = this;
}

BrowserClient::~BrowserClient() {
}

BrowserContext* BrowserClient::browser_context() {
  return browser_main_parts_->browser_context();
}

NotificationPresenter* BrowserClient::notification_presenter() {
#if defined(OS_MACOSX)
  if (!notification_presenter_)
    notification_presenter_.reset(NotificationPresenter::Create());
#endif
  return notification_presenter_.get();
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

void BrowserClient::ShowDesktopNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id,
    bool worker) {
  auto presenter = notification_presenter();
  if (!presenter)
    return;
  presenter->ShowNotification(params, render_process_id, render_view_id);
}

void BrowserClient::CancelDesktopNotification(
    int render_process_id,
    int render_view_id,
    int notification_id) {
  auto presenter = notification_presenter();
  if (!presenter)
    return;
  presenter->CancelNotification(render_process_id, render_view_id, notification_id);
}

}
