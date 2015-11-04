// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_WIN_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_WIN_H_

#include "base/compiler_specific.h"
#include "browser/notification_presenter.h"

#include <windows.h>

namespace brightray {

class NotificationPresenterWin : public NotificationPresenter {
 public:
  NotificationPresenterWin();
  ~NotificationPresenterWin();

  // NotificationPresenter:
  void ShowNotification(
      const content::PlatformNotificationData&,
      const SkBitmap& icon,
      scoped_ptr<content::DesktopNotificationDelegate> delegate,
      base::Closure* cancel_callback) override;
  
  void RemoveNotification();

 private:

  void CancelNotification();
  void DeleteNotification();
};

}  // namespace brightray

#endif
