// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/mac/notification_center_delegate.h"

#include "browser/mac/cocoa_notification.h"
#include "browser/mac/notification_presenter_mac.h"

@implementation NotificationCenterDelegate

- (instancetype)initWithPresenter:(brightray::NotificationPresenterMac*)presenter {
  self = [super init];
  if (!self)
    return nil;

  presenter_ = presenter;
  return self;
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
        didDeliverNotification:(NSUserNotification*)notif {
  auto notification = presenter_->GetNotification(notif);
  if (notification)
    notification->NotificationDisplayed();
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
       didActivateNotification:(NSUserNotification *)notif {
  auto notification = presenter_->GetNotification(notif);
  if (notification)
    notification->NotificationClicked();
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification {
  // Display notifications even if the app is active.
  return YES;
}

@end
