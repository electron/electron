// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/notification_presenter.h"

#include <algorithm>

#include "shell/browser/notifications/notification.h"

namespace electron {

NotificationPresenter::NotificationPresenter() = default;

NotificationPresenter::~NotificationPresenter() {
  for (Notification* notification : notifications_)
    delete notification;
}

base::WeakPtr<Notification> NotificationPresenter::CreateNotification(
    NotificationDelegate* delegate,
    const std::string& notification_id) {
  Notification* notification = CreateNotificationObject(delegate);
  notification->set_notification_id(notification_id);
  notifications_.insert(notification);
  return notification->GetWeakPtr();
}

void NotificationPresenter::RemoveNotification(Notification* notification) {
  auto it = notifications_.find(notification);
  if (it == notifications_.end()) {
    return;
  }

  notifications_.erase(notification);
  delete notification;
}

void NotificationPresenter::CloseNotificationWithId(
    const std::string& notification_id) {
  auto it = std::ranges::find_if(
      notifications_, [&notification_id](const Notification* n) {
        return n->notification_id() == notification_id;
      });
  if (it != notifications_.end()) {
    Notification* notification = (*it);
    notification->Dismiss();
    notifications_.erase(notification);
  }
}

}  // namespace electron
