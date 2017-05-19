// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_LINUX_NOTIFICATION_PRESENTER_LINUX_H_
#define BRIGHTRAY_BROWSER_LINUX_NOTIFICATION_PRESENTER_LINUX_H_

#include "brightray/browser/notification_presenter.h"

namespace brightray {

class NotificationPresenterLinux : public NotificationPresenter {
 public:
  NotificationPresenterLinux();
  ~NotificationPresenterLinux();

 private:
  Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterLinux);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_LINUX_NOTIFICATION_PRESENTER_LINUX_H_
