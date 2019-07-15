// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef SHELL_BROWSER_NOTIFICATIONS_LINUX_NOTIFICATION_PRESENTER_LINUX_H_
#define SHELL_BROWSER_NOTIFICATIONS_LINUX_NOTIFICATION_PRESENTER_LINUX_H_

#include "shell/browser/notifications/notification_presenter.h"

namespace electron {

class NotificationPresenterLinux : public NotificationPresenter {
 public:
  NotificationPresenterLinux();
  ~NotificationPresenterLinux() override;

 private:
  Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterLinux);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NOTIFICATIONS_LINUX_NOTIFICATION_PRESENTER_LINUX_H_
