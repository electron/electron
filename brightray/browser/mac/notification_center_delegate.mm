// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/browser/mac/notification_center_delegate.h"

#include "brightray/browser/mac/cocoa_notification.h"
#include "brightray/browser/mac/notification_presenter_mac.h"

@implementation NotificationCenterDelegate

- (instancetype)initWithPresenter:
    (brightray::NotificationPresenterMac*)presenter {
  self = [super init];
  if (!self)
    return nil;

  presenter_ = presenter;
  return self;
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
        didDeliverNotification:(NSUserNotification*)notif {
  auto* notification = presenter_->GetNotification(notif);
  if (notification)
    notification->NotificationDisplayed();
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
       didActivateNotification:(NSUserNotification*)notif {
  auto* notification = presenter_->GetNotification(notif);

  if (getenv("ELECTRON_DEBUG_NOTIFICATIONS")) {
    LOG(INFO) << "Notification activated (" << [notif.identifier UTF8String]
              << ")";
  }

  if (notification) {
    // Ref:
    // https://developer.apple.com/documentation/foundation/nsusernotificationactivationtype?language=objc
    if (notif.activationType ==
        NSUserNotificationActivationTypeContentsClicked) {
      notification->NotificationClicked();
    } else if (notif.activationType ==
               NSUserNotificationActivationTypeActionButtonClicked) {
      notification->NotificationActivated();
    } else if (notif.activationType ==
               NSUserNotificationActivationTypeReplied) {
      notification->NotificationReplied([notif.response.string UTF8String]);
    } else {
      if (@available(macOS 10.10, *)) {
        if (notif.activationType ==
            NSUserNotificationActivationTypeAdditionalActionClicked) {
          notification->NotificationActivated(
              [notif additionalActivationAction]);
        }
      }
    }
  }
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification {
  // Display notifications even if the app is active.
  return YES;
}

@end
