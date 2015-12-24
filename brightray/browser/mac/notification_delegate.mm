// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/mac/notification_delegate.h"

#include "browser/mac/cocoa_notification.h"

@implementation NotificationDelegate

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
        didDeliverNotification:(NSUserNotification*)notif {
  auto notification = brightray::CocoaNotification::FromNSNotification(notif);
  if (notification)
    notification->NotifyDisplayed();
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
       didActivateNotification:(NSUserNotification *)notif {
  auto notification = brightray::CocoaNotification::FromNSNotification(notif);
  if (notification)
    notification->NotifyClick();
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification {
  // Display notifications even if the app is active.
  return YES;
}

@end
