// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/platform_notification_service.h"

#include "browser/browser_client.h"
#include "browser/notification.h"
#include "browser/notification_delegate_adapter.h"
#include "browser/notification_presenter.h"
#include "content/public/common/platform_notification_data.h"
#include "content/public/common/notification_resources.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace brightray {

namespace {

void RemoveNotification(base::WeakPtr<Notification> notification) {
  if (notification)
    notification->Dismiss();
}

void OnWebNotificationAllowed(
    int render_process_id,
    brightray::BrowserClient* browser_client,
    const SkBitmap& icon,
    const content::PlatformNotificationData& data,
    std::unique_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback,
    bool allowed) {
  if (!allowed)
    return;
  auto presenter = browser_client->GetNotificationPresenter();
  if (!presenter)
    return;
  std::unique_ptr<NotificationDelegateAdapter> adapter(
      new NotificationDelegateAdapter(std::move(delegate)));
  auto notification = presenter->CreateNotification(adapter.get());
  if (notification) {
    bool silent = data.silent;
    if (!silent) {
      silent = browser_client->WebContentsAudioMuted(render_process_id);
    }
    ignore_result(adapter.release());  // it will release itself automatically.
    notification->Show(data.title, data.body, data.tag, data.icon, icon, silent);
    *cancel_callback = base::Bind(&RemoveNotification, notification);
  }
}

}  // namespace

PlatformNotificationService::PlatformNotificationService(
    BrowserClient* browser_client)
    : browser_client_(browser_client),
      render_process_id_(-1) {
}

PlatformNotificationService::~PlatformNotificationService() {}

blink::WebNotificationPermission PlatformNotificationService::CheckPermissionOnUIThread(
    content::BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  render_process_id_ = render_process_id;
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
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources,
    std::unique_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  browser_client_->WebNotificationAllowed(
      render_process_id_,
      base::Bind(&OnWebNotificationAllowed,
                 render_process_id_,
                 browser_client_,
                 notification_resources.notification_icon,
                 notification_data,
                 base::Passed(&delegate),
                 cancel_callback));
}

void PlatformNotificationService::DisplayPersistentNotification(
    content::BrowserContext* browser_context,
    int64_t persistent_notification_id,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources) {
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
