// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BROWSER_MAC_COCOA_NOTIFICATION_H_
#define BROWSER_MAC_COCOA_NOTIFICATION_H_

#include <set>

#import <Foundation/Foundation.h>

#include "base/mac/scoped_nsobject.h"
#include "base/memory/scoped_ptr.h"
#include "browser/notification.h"
#include "content/public/browser/desktop_notification_delegate.h"

@class NotificationDelegate;

namespace brightray {

class CocoaNotification : public Notification {
 public:
  static CocoaNotification* FromNSNotification(
      NSUserNotification* notification);

  CocoaNotification(
      scoped_ptr<content::DesktopNotificationDelegate> delegate);
  ~CocoaNotification();

  void ShowNotification(const base::string16& title,
                        const base::string16& msg,
                        const SkBitmap& icon) override;
  void DismissNotification() override;

  void NotifyDisplayed();
  void NotifyClick();

 private:
  static void Cleanup();

  scoped_ptr<content::DesktopNotificationDelegate> delegate_;
  base::scoped_nsobject<NSUserNotification> notification_;

  static base::scoped_nsobject<NotificationDelegate> notification_delegate_;
  static std::set<CocoaNotification*> notifications_;

  DISALLOW_COPY_AND_ASSIGN(CocoaNotification);
};

}  // namespace brightray

#endif  // BROWSER_MAC_COCOA_NOTIFICATION_H_
