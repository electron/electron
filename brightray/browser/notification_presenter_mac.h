// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_

#import "browser/notification_presenter.h"

#import "base/mac/scoped_nsobject.h"
#import <map>

@class BRYUserNotificationCenterDelegate;

namespace brightray {

class NotificationPresenterMac : public NotificationPresenter {
 public:
  NotificationPresenterMac();
  ~NotificationPresenterMac();

  virtual void ShowNotification(
      const content::ShowDesktopNotificationHostMsgParams&,
      scoped_ptr<content::DesktopNotificationDelegate> delegate,
      base::Closure* cancel_callback) override;

  // Get the delegate accroding from the notification object.
  content::DesktopNotificationDelegate* GetDelegateFromNotification(
      NSUserNotification* notification);

  // Remove the notification object accroding to its delegate.
  void RemoveNotification(content::DesktopNotificationDelegate* delegate);

 private:
  void CancelNotification(content::DesktopNotificationDelegate* delegate);

  // The userInfo of NSUserNotification can not store pointers (because they are
  // not in property list), so we have to track them in a C++ map.
  // Also notice that the delegate acts as "ID" or "Key", because it is certain
  // that each notification has a unique delegate.
  typedef std::map<content::DesktopNotificationDelegate*, base::scoped_nsobject<NSUserNotification>>
      NotificationsMap;
  NotificationsMap notifications_map_;

  base::scoped_nsobject<BRYUserNotificationCenterDelegate> delegate_;
};

}  // namespace brightray

#endif
