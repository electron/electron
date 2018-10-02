// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/platform_notification_service.h"

#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/browser_client.h"
#include "brightray/browser/notification.h"
#include "brightray/browser/notification_delegate.h"
#include "brightray/browser/notification_presenter.h"
#include "content/public/browser/notification_event_dispatcher.h"
#include "content/public/common/notification_resources.h"
#include "content/public/common/platform_notification_data.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace brightray {

namespace {

void OnWebNotificationAllowed(base::WeakPtr<Notification> notification,
                              const SkBitmap& icon,
                              const content::PlatformNotificationData& data,
                              bool audio_muted,
                              bool allowed) {
  if (!notification)
    return;
  if (allowed) {
    brightray::NotificationOptions options;
    options.title = data.title;
    options.msg = data.body;
    options.tag = data.tag;
    options.icon_url = data.icon;
    options.icon = icon;
    options.silent = audio_muted ? true : data.silent;
    options.has_reply = false;
    notification->Show(options);
  } else {
    notification->Destroy();
  }
}

class NotificationDelegateImpl final : public brightray::NotificationDelegate {
 public:
  explicit NotificationDelegateImpl(const std::string& notification_id)
      : notification_id_(notification_id) {}

  void NotificationDestroyed() override { delete this; }

  void NotificationClick() override {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentClickEvent(notification_id_, base::DoNothing());
  }

  void NotificationClosed() override {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentCloseEvent(notification_id_, base::DoNothing());
  }

  void NotificationDisplayed() override {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentShowEvent(notification_id_);
  }

 private:
  std::string notification_id_;

  DISALLOW_COPY_AND_ASSIGN(NotificationDelegateImpl);
};

}  // namespace

PlatformNotificationService::PlatformNotificationService(
    BrowserClient* browser_client)
    : browser_client_(browser_client), render_process_id_(-1) {}

PlatformNotificationService::~PlatformNotificationService() {}

void PlatformNotificationService::DisplayNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources) {
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;
  NotificationDelegateImpl* delegate =
      new NotificationDelegateImpl(notification_id);
  auto notification = presenter->CreateNotification(delegate, notification_id);
  if (notification) {
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
    const content::NotificationResources& notification_resources) {}

void PlatformNotificationService::ClosePersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id) {}

void PlatformNotificationService::CloseNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id) {
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;
  presenter->CloseNotificationWithId(notification_id);
}

void PlatformNotificationService::GetDisplayedNotifications(
    content::BrowserContext* browser_context,
    const DisplayedNotificationsCallback& callback) {}

int64_t PlatformNotificationService::ReadNextPersistentNotificationId(
    content::BrowserContext* browser_context) {
  NOTREACHED() << "TODO";
  return 0;
}

}  // namespace brightray
