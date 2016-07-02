// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_BROWSER_CLIENT_H_
#define BRIGHTRAY_BROWSER_BROWSER_CLIENT_H_

#include "browser/net_log.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"

namespace brightray {

class BrowserContext;
class BrowserMainParts;
class NotificationPresenter;
class PlatformNotificationService;

class BrowserClient : public content::ContentBrowserClient {
 public:
  static BrowserClient* Get();

  BrowserClient();
  ~BrowserClient();

  BrowserMainParts* browser_main_parts() { return browser_main_parts_; }

  NotificationPresenter* GetNotificationPresenter();

  // Subclasses should override this to enable or disable WebNotification.
  virtual void WebNotificationAllowed(
      int render_process_id,
      const base::Callback<void(bool, bool)>& callback) {
    callback.Run(false, true);
  }

  // Subclasses that override this (e.g., to provide their own protocol
  // handlers) should call this implementation after doing their own work.
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams&) override;
  content::MediaObserver* GetMediaObserver() override;
  content::PlatformNotificationService* GetPlatformNotificationService() override;
  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) override;
  net::NetLog* GetNetLog() override;
  base::FilePath GetDefaultDownloadDirectory() override;
  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

 protected:
  // Subclasses should override this to provide their own BrowserMainParts
  // implementation. The lifetime of the returned instance is managed by the
  // caller.
  virtual BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&);

 private:
  BrowserMainParts* browser_main_parts_;
  NetLog net_log_;

  std::unique_ptr<PlatformNotificationService> notification_service_;
  std::unique_ptr<NotificationPresenter> notification_presenter_;

  DISALLOW_COPY_AND_ASSIGN(BrowserClient);
};

}  // namespace brightray

#endif
