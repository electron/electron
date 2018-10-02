// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_
#define BRIGHTRAY_BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_

#include <set>
#include <string>

#include "content/public/browser/browser_context.h"
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
  void DisplayNotification(
      content::BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const content::NotificationResources& notification_resources) override;
  void DisplayPersistentNotification(
      content::BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& service_worker_scope,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const content::NotificationResources& notification_resources) override;
  void ClosePersistentNotification(content::BrowserContext* browser_context,
                                   const std::string& notification_id) override;
  void CloseNotification(content::BrowserContext* browser_context,
                         const std::string& notification_id) override;
  void GetDisplayedNotifications(
      content::BrowserContext* browser_context,
      const DisplayedNotificationsCallback& callback) override;
  int64_t ReadNextPersistentNotificationId(
      content::BrowserContext* browser_context) override;

 private:
  BrowserClient* browser_client_;
  int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(PlatformNotificationService);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_
