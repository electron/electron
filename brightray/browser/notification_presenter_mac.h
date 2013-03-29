// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_

#import "browser/notification_presenter.h"

#import "base/memory/scoped_nsobject.h"
#import <map>

@class BRYUserNotificationCenterDelegate;

namespace brightray {

class NotificationPresenterMac : public NotificationPresenter {
 public:
  NotificationPresenterMac();
  ~NotificationPresenterMac();

  virtual void ShowNotification(
      const content::ShowDesktopNotificationHostMsgParams&,
      int render_process_id,
      int render_view_id) OVERRIDE;
  virtual void CancelNotification(
      int render_process_id,
      int render_view_id,
      int notification_id) OVERRIDE;

 private:
  std::map<std::string, scoped_nsobject<NSUserNotification>> notification_map_;
  scoped_nsobject<BRYUserNotificationCenterDelegate> delegate_;
};

}

#endif
