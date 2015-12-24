// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_

#include "browser/notification_presenter.h"

namespace brightray {

class NotificationPresenterMac : public NotificationPresenter {
 public:
  NotificationPresenterMac();
  ~NotificationPresenterMac();

  // NotificationPresenter:
  void ShowNotification(
      const content::PlatformNotificationData&,
      const SkBitmap& icon,
      scoped_ptr<content::DesktopNotificationDelegate> delegate,
      base::Closure* cancel_callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterMac);
};

}  // namespace brightray

#endif
