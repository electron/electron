// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ATOM_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_
#define ATOM_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_

#include <set>
#include <string>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/platform_notification_service.h"

namespace atom {

class AtomBrowserClient;

class PlatformNotificationService
    : public content::PlatformNotificationService {
 public:
  explicit PlatformNotificationService(AtomBrowserClient* browser_client);
  ~PlatformNotificationService() override;

 protected:
  // content::PlatformNotificationService:
  void DisplayNotification(
      content::RenderProcessHost* render_process_host,
      content::BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& origin,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) override;
  void DisplayPersistentNotification(
      content::BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& service_worker_scope,
      const GURL& origin,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) override;
  void ClosePersistentNotification(content::BrowserContext* browser_context,
                                   const std::string& notification_id) override;
  void CloseNotification(content::BrowserContext* browser_context,
                         const std::string& notification_id) override;
  void GetDisplayedNotifications(
      content::BrowserContext* browser_context,
      const DisplayedNotificationsCallback& callback) override;
  int64_t ReadNextPersistentNotificationId(
      content::BrowserContext* browser_context) override;
  void RecordNotificationUkmEvent(
      content::BrowserContext* browser_context,
      const content::NotificationDatabaseData& data) override;

 private:
  AtomBrowserClient* browser_client_;

  DISALLOW_COPY_AND_ASSIGN(PlatformNotificationService);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_
