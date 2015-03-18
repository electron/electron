// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_
#define BROWSER_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_

#include "base/memory/singleton.h"
#include "content/public/browser/platform_notification_service.h"

namespace brightray {

class NotificationPresenter;

class PlatformNotificationServiceImpl
    : public content::PlatformNotificationService {
 public:
  // Returns the active instance of the service in the browser process. Safe to
  // be called from any thread.
  static PlatformNotificationServiceImpl* GetInstance();

  NotificationPresenter* notification_presenter();

 private:
  friend struct DefaultSingletonTraits<PlatformNotificationServiceImpl>;

  PlatformNotificationServiceImpl();
  ~PlatformNotificationServiceImpl() override;

  // content::PlatformNotificationService:
  virtual blink::WebNotificationPermission CheckPermission(
      content::ResourceContext* resource_context,
      const GURL& origin,
      int render_process_id) override;
  virtual void DisplayNotification(
      content::BrowserContext* browser_context,
      const GURL& origin,
      const SkBitmap& icon,
      const content::PlatformNotificationData& notification_data,
      scoped_ptr<content::DesktopNotificationDelegate> delegate,
      int render_process_id,
      base::Closure* cancel_callback) override;
  virtual void DisplayPersistentNotification(
      content::BrowserContext* browser_context,
      int64 service_worker_registration_id,
      const GURL& origin,
      const SkBitmap& icon,
      const content::PlatformNotificationData& notification_data,
      int render_process_id) override;
  virtual void ClosePersistentNotification(
      content::BrowserContext* browser_context,
      const std::string& persistent_notification_id) override;

  scoped_ptr<NotificationPresenter> notification_presenter_;

  DISALLOW_COPY_AND_ASSIGN(PlatformNotificationServiceImpl);
};

}  // namespace brightray

#endif  // BROWSER_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_
