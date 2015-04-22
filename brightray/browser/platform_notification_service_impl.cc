// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/platform_notification_service_impl.h"

#include "browser/notification_presenter.h"

#include "content/public/browser/desktop_notification_delegate.h"

namespace brightray {

// static
PlatformNotificationServiceImpl*
PlatformNotificationServiceImpl::GetInstance() {
  return Singleton<PlatformNotificationServiceImpl>::get();
}

PlatformNotificationServiceImpl::PlatformNotificationServiceImpl() {}
PlatformNotificationServiceImpl::~PlatformNotificationServiceImpl() {}

NotificationPresenter* PlatformNotificationServiceImpl::notification_presenter() {
#if defined(OS_MACOSX) || defined(OS_LINUX)
  if (!notification_presenter_)
    notification_presenter_.reset(NotificationPresenter::Create());
#endif
  return notification_presenter_.get();
}

blink::WebNotificationPermission PlatformNotificationServiceImpl::CheckPermissionOnUIThread(
    content::BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  return blink::WebNotificationPermissionAllowed;
}

blink::WebNotificationPermission PlatformNotificationServiceImpl::CheckPermissionOnIOThread(
    content::ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  return blink::WebNotificationPermissionAllowed;
}

void PlatformNotificationServiceImpl::DisplayNotification(
    content::BrowserContext* browser_context,
    const GURL& origin,
    const SkBitmap& icon,
    const content::PlatformNotificationData& notification_data,
    scoped_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  auto presenter = notification_presenter();
  if (presenter)
    presenter->ShowNotification(notification_data, icon, delegate.Pass(), cancel_callback);
}

void PlatformNotificationServiceImpl::DisplayPersistentNotification(
    content::BrowserContext* browser_context,
    int64_t service_worker_registration_id,
    const GURL& origin,
    const SkBitmap& icon,
    const content::PlatformNotificationData& notification_data) {
}

void PlatformNotificationServiceImpl::ClosePersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& persistent_notification_id) {
}

}  // namespace brightray
