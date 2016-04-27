// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_
#define BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_

#include "content/public/browser/platform_notification_service.h"

namespace brightray {

class BrowserClient;

class PlatformNotificationService
    : public content::PlatformNotificationService {
 public:
  explicit PlatformNotificationService(BrowserClient* browser_client);
  ~PlatformNotificationService() override;

 protected:
  // content::PlatformNotificationService:
  blink::WebNotificationPermission CheckPermissionOnUIThread(
      content::BrowserContext* browser_context,
      const GURL& origin,
      int render_process_id) override;
  blink::WebNotificationPermission CheckPermissionOnIOThread(
      content::ResourceContext* resource_context,
      const GURL& origin,
      int render_process_id) override;
  void DisplayNotification(content::BrowserContext* browser_context,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const content::NotificationResources& notification_resources,
      scoped_ptr<content::DesktopNotificationDelegate> delegate,
      base::Closure* cancel_callback) override;
  void DisplayPersistentNotification(
      content::BrowserContext* browser_context,
      int64_t persistent_notification_id,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const content::NotificationResources& notification_resources) override;
  void ClosePersistentNotification(
      content::BrowserContext* browser_context,
      int64_t persistent_notification_id) override;
  bool GetDisplayedPersistentNotifications(
      content::BrowserContext* browser_context,
      std::set<std::string>* displayed_notifications) override;

 private:
  BrowserClient* browser_client_;
  int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(PlatformNotificationService);
};

}  // namespace brightray

#endif  // BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_
