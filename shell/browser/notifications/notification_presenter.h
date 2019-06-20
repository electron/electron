// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_PRESENTER_H_
#define SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_PRESENTER_H_

#include <set>
#include <string>

#include "base/memory/weak_ptr.h"

namespace electron {

class Notification;
class NotificationDelegate;

class NotificationPresenter {
 public:
  static NotificationPresenter* Create();

  virtual ~NotificationPresenter();

  base::WeakPtr<Notification> CreateNotification(
      NotificationDelegate* delegate,
      const std::string& notification_id);
  void CloseNotificationWithId(const std::string& notification_id);

  std::set<Notification*> notifications() const { return notifications_; }

 protected:
  NotificationPresenter();
  virtual Notification* CreateNotificationObject(
      NotificationDelegate* delegate) = 0;

 private:
  friend class Notification;

  void RemoveNotification(Notification* notification);

  std::set<Notification*> notifications_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenter);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_PRESENTER_H_
