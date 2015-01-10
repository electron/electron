// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/browser_client.h"

#include "browser/browser_context.h"
#include "browser/browser_main_parts.h"
#include "browser/devtools_manager_delegate.h"
#include "browser/media/media_capture_devices_dispatcher.h"
#include "browser/notification_presenter.h"

#include "base/base_paths.h"
#include "base/path_service.h"
#include "content/public/common/url_constants.h"

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
#if defined(OS_MACOSX) || defined(OS_LINUX)
  if (!notification_presenter_)
    notification_presenter_.reset(NotificationPresenter::Create());
#endif
  return notification_presenter_.get();
}

BrowserMainParts* BrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  return new BrowserMainParts;
}

content::BrowserMainParts* BrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  DCHECK(!browser_main_parts_);
  browser_main_parts_ = OverrideCreateBrowserMainParts(parameters);
  return browser_main_parts_;
}

net::URLRequestContextGetter* BrowserClient::CreateRequestContext(
    content::BrowserContext* browser_context,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors) {
  auto context = static_cast<BrowserContext*>(browser_context);
  return context->CreateRequestContext(protocol_handlers, protocol_interceptors.Pass());
}

void BrowserClient::ShowDesktopNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    content::BrowserContext* browser_context,
    int render_process_id,
    scoped_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  auto presenter = notification_presenter();
  if (presenter)
    presenter->ShowNotification(params, delegate.Pass(), cancel_callback);
}

content::MediaObserver* BrowserClient::GetMediaObserver() {
  return MediaCaptureDevicesDispatcher::GetInstance();
}

void BrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
  additional_schemes->push_back(content::kChromeDevToolsScheme);
  additional_schemes->push_back(content::kChromeUIScheme);
}

base::FilePath BrowserClient::GetDefaultDownloadDirectory() {
  // ~/Downloads
  base::FilePath path;
  if (PathService::Get(base::DIR_HOME, &path))
    path = path.Append(FILE_PATH_LITERAL("Downloads"));

  return path;
}

content::DevToolsManagerDelegate* BrowserClient::GetDevToolsManagerDelegate() {
  return new DevToolsManagerDelegate;
}

}  // namespace brightray
