// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/browser/notification_presenter.h"

#include <algorithm>

#include "brightray/browser/notification.h"

namespace brightray {

NotificationPresenter::NotificationPresenter() {}

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
  notifications_.erase(notification);
  delete notification;
}

void NotificationPresenter::CloseNotificationWithId(
    const std::string& notification_id) {
  auto it = std::find_if(notifications_.begin(), notifications_.end(),
                         [&notification_id](const Notification* n) {
                           return n->notification_id() == notification_id;
                         });
  if (it != notifications_.end())
    (*it)->Dismiss();
}

}  // namespace brightray
