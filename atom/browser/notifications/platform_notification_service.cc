// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "atom/browser/notifications/platform_notification_service.h"

#include "atom/browser/atom_browser_client.h"
#include "atom/browser/notifications/notification.h"
#include "atom/browser/notifications/notification_delegate.h"
#include "atom/browser/notifications/notification_presenter.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/notification_event_dispatcher.h"
#include "content/public/browser/render_process_host.h"
#include "third_party/blink/public/common/notifications/notification_resources.h"
#include "third_party/blink/public/common/notifications/platform_notification_data.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace atom {

namespace {

void OnWebNotificationAllowed(base::WeakPtr<Notification> notification,
                              const SkBitmap& icon,
                              const blink::PlatformNotificationData& data,
                              bool audio_muted,
                              bool allowed) {
  if (!notification)
    return;
  if (allowed) {
    atom::NotificationOptions options;
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

class NotificationDelegateImpl final : public atom::NotificationDelegate {
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
    AtomBrowserClient* browser_client)
    : browser_client_(browser_client) {}

PlatformNotificationService::~PlatformNotificationService() {}

void PlatformNotificationService::DisplayNotification(
    content::RenderProcessHost* render_process_host,
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const blink::PlatformNotificationData& notification_data,
    const blink::NotificationResources& notification_resources) {
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;
  NotificationDelegateImpl* delegate =
      new NotificationDelegateImpl(notification_id);
  auto notification = presenter->CreateNotification(delegate, notification_id);
  if (notification) {
    browser_client_->WebNotificationAllowed(
        render_process_host->GetID(),
        base::Bind(&OnWebNotificationAllowed, notification,
                   notification_resources.notification_icon,
                   notification_data));
  }
}

void PlatformNotificationService::DisplayPersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& service_worker_scope,
    const GURL& origin,
    const blink::PlatformNotificationData& notification_data,
    const blink::NotificationResources& notification_resources) {}

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
  // Electron doesn't support persistent notifications.
  return 0;
}

void PlatformNotificationService::RecordNotificationUkmEvent(
    content::BrowserContext* browser_context,
    const content::NotificationDatabaseData& data) {}

}  // namespace atom
