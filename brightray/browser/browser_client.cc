// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/browser_client.h"

#include "base/path_service.h"
#include "browser/browser_context.h"
#include "browser/browser_main_parts.h"
#include "browser/devtools_manager_delegate.h"
#include "browser/media/media_capture_devices_dispatcher.h"
#include "browser/notification_presenter.h"
#include "browser/platform_notification_service.h"
#include "content/public/common/url_constants.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace brightray {

namespace {

BrowserClient* g_browser_client;

}

BrowserClient* BrowserClient::Get() {
  return g_browser_client;
}

BrowserClient::BrowserClient()
    : browser_main_parts_(nullptr) {
  DCHECK(!g_browser_client);
  g_browser_client = this;
}

BrowserClient::~BrowserClient() {
}

BrowserContext* BrowserClient::browser_context() {
  return browser_main_parts_->browser_context();
}

NotificationPresenter* BrowserClient::GetNotificationPresenter() {
  #if defined(OS_WIN)
  // Bail out if on Windows 7 or even lower, no operating will follow
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return nullptr;
  #endif

  if (!notification_presenter_) {
    // Create a new presenter if on OS X, Linux, or Windows 8+
    notification_presenter_.reset(NotificationPresenter::Create());
  }
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

content::MediaObserver* BrowserClient::GetMediaObserver() {
  return MediaCaptureDevicesDispatcher::GetInstance();
}

content::PlatformNotificationService* BrowserClient::GetPlatformNotificationService() {
  if (!notification_service_)
    notification_service_.reset(new PlatformNotificationService(this));
  return notification_service_.get();
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
