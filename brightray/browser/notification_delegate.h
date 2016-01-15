// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BROWSER_NOTIFICATION_DELEGATE_H_
#define BROWSER_NOTIFICATION_DELEGATE_H_

#include "content/public/browser/desktop_notification_delegate.h"

namespace brightray {

class NotificationDelegate : public content::DesktopNotificationDelegate {
 public:
  // The native Notification object is destroyed.
  virtual void NotificationDestroyed() {}

  // Failed to send the notification.
  virtual void NotificationFailed() {}
};

}  // namespace brightray

#endif  // BROWSER_NOTIFICATION_DELEGATE_H_
