// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NOTIFICATIONS_NOTIFICATION_DELEGATE_H_
#define ATOM_BROWSER_NOTIFICATIONS_NOTIFICATION_DELEGATE_H_

#include <string>

namespace atom {

class NotificationDelegate {
 public:
  // The native Notification object is destroyed.
  virtual void NotificationDestroyed() {}

  // Failed to send the notification.
  virtual void NotificationFailed() {}

  // Notification was replied to
  virtual void NotificationReplied(const std::string& reply) {}
  virtual void NotificationAction(int index) {}

  virtual void NotificationClick() {}
  virtual void NotificationClosed() {}
  virtual void NotificationDisplayed() {}

 protected:
  NotificationDelegate() = default;
  ~NotificationDelegate() = default;
};

}  // namespace atom

#endif  // ATOM_BROWSER_NOTIFICATIONS_NOTIFICATION_DELEGATE_H_
