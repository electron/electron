// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_PRESENTER_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_PRESENTER_H_

#include <memory>
#include <set>
#include <string>

#include "base/memory/weak_ptr.h"

namespace electron {

class Notification;
class NotificationDelegate;

class NotificationPresenter {
 public:
  static std::unique_ptr<NotificationPresenter> Create();

  virtual ~NotificationPresenter();

  base::WeakPtr<Notification> CreateNotification(
      NotificationDelegate* delegate,
      const std::string& notification_id);
  void CloseNotificationWithId(const std::string& notification_id);

  std::set<Notification*> notifications() const { return notifications_; }

  // disable copy
  NotificationPresenter(const NotificationPresenter&) = delete;
  NotificationPresenter& operator=(const NotificationPresenter&) = delete;

 protected:
  NotificationPresenter();
  virtual Notification* CreateNotificationObject(
      NotificationDelegate* delegate) = 0;

 private:
  friend class Notification;

  void RemoveNotification(Notification* notification);

  std::set<Notification*> notifications_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_PRESENTER_H_
