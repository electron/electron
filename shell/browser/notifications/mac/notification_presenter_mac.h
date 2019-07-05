// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_PRESENTER_MAC_H_
#define SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_PRESENTER_MAC_H_

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/notifications/mac/notification_center_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"

namespace electron {

class CocoaNotification;

class NotificationPresenterMac : public NotificationPresenter {
 public:
  CocoaNotification* GetNotification(NSUserNotification* notif);

  NotificationPresenterMac();
  ~NotificationPresenterMac() override;

 private:
  Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  base::scoped_nsobject<NotificationCenterDelegate>
      notification_center_delegate_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterMac);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_PRESENTER_MAC_H_
