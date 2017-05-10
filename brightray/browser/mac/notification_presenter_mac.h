// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_

#include "base/mac/scoped_nsobject.h"
#include "browser/mac/notification_center_delegate.h"
#include "browser/notification_presenter.h"

namespace brightray {

class CocoaNotification;

class NotificationPresenterMac : public NotificationPresenter {
 public:
  CocoaNotification* GetNotification(NSUserNotification* notif);

  NotificationPresenterMac();
  ~NotificationPresenterMac();

 private:
  Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  base::scoped_nsobject<NotificationCenterDelegate>
      notification_center_delegate_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterMac);
};

}  // namespace brightray

#endif
