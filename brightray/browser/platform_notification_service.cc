// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/platform_notification_service.h"

#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/browser_client.h"
#include "brightray/browser/notification.h"
#include "brightray/browser/notification_delegate_adapter.h"
#include "brightray/browser/notification_presenter.h"
#include "content/public/common/notification_resources.h"
#include "content/public/common/platform_notification_data.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace brightray {

namespace {

void RemoveNotification(base::WeakPtr<Notification> notification) {
  if (notification)
    notification->Dismiss();
}

void OnWebNotificationAllowed(base::WeakPtr<Notification> notification,
                              const SkBitmap& icon,
                              const content::PlatformNotificationData& data,
                              bool audio_muted,
                              bool allowed) {
  if (!notification)
    return;
  if (allowed)
    notification->Show(data.title, data.body, data.tag, data.icon, icon,
                       audio_muted ? true : data.silent, false,
                       base::UTF8ToUTF16(""));
  else
    notification->Destroy();
}

}  // namespace

PlatformNotificationService::PlatformNotificationService(
    BrowserClient* browser_client)
    : browser_client_(browser_client),
      render_process_id_(-1) {
}

PlatformNotificationService::~PlatformNotificationService() {}

blink::mojom::PermissionStatus
PlatformNotificationService::CheckPermissionOnUIThread(
    content::BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  render_process_id_ = render_process_id;
  return blink::mojom::PermissionStatus::GRANTED;
}

blink::mojom::PermissionStatus
PlatformNotificationService::CheckPermissionOnIOThread(
    content::ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  return blink::mojom::PermissionStatus::GRANTED;
}

void PlatformNotificationService::DisplayNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources,
    std::unique_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  auto presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;
  std::unique_ptr<NotificationDelegateAdapter> adapter(
      new NotificationDelegateAdapter(std::move(delegate)));
  auto notification = presenter->CreateNotification(adapter.get());
  if (notification) {
    ignore_result(adapter.release());  // it will release itself automatically.
    *cancel_callback = base::Bind(&RemoveNotification, notification);
    browser_client_->WebNotificationAllowed(
        render_process_id_, base::Bind(&OnWebNotificationAllowed, notification,
                                       notification_resources.notification_icon,
                                       notification_data));
  }
}

void PlatformNotificationService::DisplayPersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& service_worker_scope,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources) {
}

void PlatformNotificationService::ClosePersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id) {
}

bool PlatformNotificationService::GetDisplayedNotifications(
    content::BrowserContext* browser_context,
    std::set<std::string>* displayed_notifications) {
  return false;
}

}  // namespace brightray
