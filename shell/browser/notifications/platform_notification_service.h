// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef SHELL_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_
#define SHELL_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_

#include <set>
#include <string>

#include "content/public/browser/platform_notification_service.h"

namespace electron {

class ElectronBrowserClient;

class PlatformNotificationService
    : public content::PlatformNotificationService {
 public:
  explicit PlatformNotificationService(ElectronBrowserClient* browser_client);
  ~PlatformNotificationService() override;

 protected:
  // content::PlatformNotificationService:
  void DisplayNotification(
      content::RenderProcessHost* render_process_host,
      const std::string& notification_id,
      const GURL& origin,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) override;
  void DisplayPersistentNotification(
      const std::string& notification_id,
      const GURL& service_worker_scope,
      const GURL& origin,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) override;
  void ClosePersistentNotification(const std::string& notification_id) override;
  void CloseNotification(const std::string& notification_id) override;
  void GetDisplayedNotifications(
      DisplayedNotificationsCallback callback) override;
  int64_t ReadNextPersistentNotificationId() override;
  void RecordNotificationUkmEvent(
      const content::NotificationDatabaseData& data) override;
  void ScheduleTrigger(base::Time timestamp) override;
  base::Time ReadNextTriggerTimestamp() override;

 private:
  ElectronBrowserClient* browser_client_;

  DISALLOW_COPY_AND_ASSIGN(PlatformNotificationService);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_
