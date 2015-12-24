// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/platform_notification_service.h"

#include "browser/browser_client.h"
#include "browser/notification_presenter.h"
#include "content/public/browser/desktop_notification_delegate.h"

namespace brightray {

PlatformNotificationService::PlatformNotificationService(
    BrowserClient* browser_client)
    : browser_client_(browser_client) {
}

PlatformNotificationService::~PlatformNotificationService() {}

blink::WebNotificationPermission PlatformNotificationService::CheckPermissionOnUIThread(
    content::BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  return blink::WebNotificationPermissionAllowed;
}

blink::WebNotificationPermission PlatformNotificationService::CheckPermissionOnIOThread(
    content::ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  return blink::WebNotificationPermissionAllowed;
}

void PlatformNotificationService::DisplayNotification(
    content::BrowserContext* browser_context,
    const GURL& origin,
    const SkBitmap& icon,
    const content::PlatformNotificationData& notification_data,
    scoped_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  auto presenter = browser_client_->GetNotificationPresenter();
  if (presenter)
    presenter->ShowNotification(notification_data, icon, delegate.Pass(), cancel_callback);
}

void PlatformNotificationService::DisplayPersistentNotification(
    content::BrowserContext* browser_context,
    int64_t service_worker_registration_id,
    const GURL& origin,
    const SkBitmap& icon,
    const content::PlatformNotificationData& notification_data) {
}

void PlatformNotificationService::ClosePersistentNotification(
    content::BrowserContext* browser_context,
    int64_t persistent_notification_id) {
}

bool PlatformNotificationService::GetDisplayedPersistentNotifications(
    content::BrowserContext* browser_context,
    std::set<std::string>* displayed_notifications) {
  return false;
}

}  // namespace brightray
