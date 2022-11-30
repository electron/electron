// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/mac/notification_center_delegate.h"

#include <string>

#include "base/logging.h"
#include "shell/browser/notifications/mac/cocoa_notification.h"
#include "shell/browser/notifications/mac/notification_presenter_mac.h"

@implementation NotificationCenterDelegate

- (instancetype)initWithPresenter:
    (electron::NotificationPresenterMac*)presenter {
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
      if (notif.activationType ==
          NSUserNotificationActivationTypeAdditionalActionClicked) {
        notification->NotificationActivated([notif additionalActivationAction]);
      }
    }
  }
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification {
  // Display notifications even if the app is active.
  return YES;
}

#if !IS_MAS_BUILD()
// This undocumented method notifies us if a user closes "Alert" notifications
// https://chromium.googlesource.com/chromium/src/+/lkgr/chrome/browser/notifications/notification_platform_bridge_mac.mm
- (void)userNotificationCenter:(NSUserNotificationCenter*)center
               didDismissAlert:(NSUserNotification*)notif {
  auto* notification = presenter_->GetNotification(notif);
  if (notification)
    notification->NotificationDismissed();
}
#endif

#if !IS_MAS_BUILD()
// This undocumented method notifies us if a user closes "Banner" notifications
// https://github.com/mozilla/gecko-dev/blob/master/widget/cocoa/OSXNotificationCenter.mm
- (void)userNotificationCenter:(NSUserNotificationCenter*)center
    didRemoveDeliveredNotifications:(NSArray*)notifications {
  for (NSUserNotification* notif in notifications) {
    auto* notification = presenter_->GetNotification(notif);
    if (notification)
      notification->NotificationDismissed();
  }
}
#endif

@end
