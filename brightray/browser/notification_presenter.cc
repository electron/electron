// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/browser/notification_presenter.h"

#include <vector>

#include "brightray/browser/notification.h"

namespace brightray {

NotificationPresenter::NotificationPresenter() {
}

NotificationPresenter::~NotificationPresenter() {
  for (Notification* notification : notifications_)
    delete notification;
}

base::WeakPtr<Notification> NotificationPresenter::CreateNotification(
    NotificationDelegate* delegate) {
  Notification* notification = CreateNotificationObject(delegate);
  notifications_.insert(notification);
  return notification->GetWeakPtr();
}

void NotificationPresenter::RemoveAllNotifications() {
  // Copy notifications to avoid iterator invalidation
  std::vector<Notification*> notifications_to_remove(
      notifications_.begin(), notifications_.end());
  for (Notification* notification : notifications_to_remove)
    RemoveNotification(notification);
}

void NotificationPresenter::RemoveNotification(Notification* notification) {
  notifications_.erase(notification);
  delete notification;
}

}  // namespace brightray
