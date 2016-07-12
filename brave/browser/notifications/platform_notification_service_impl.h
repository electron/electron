// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRAVE_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_
#define BRAVE_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_

#include "base/memory/singleton.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/platform_notification_service.h"

namespace brave {

class BrowserClient;

class PlatformNotificationServiceImpl
    : public content::PlatformNotificationService {
 public:
  explicit PlatformNotificationServiceImpl();
  ~PlatformNotificationServiceImpl() override;

  static PlatformNotificationServiceImpl* GetInstance();

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
      std::unique_ptr<content::DesktopNotificationDelegate> delegate,
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
  friend struct base::DefaultSingletonTraits<PlatformNotificationServiceImpl>;

  DISALLOW_COPY_AND_ASSIGN(PlatformNotificationServiceImpl);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_
