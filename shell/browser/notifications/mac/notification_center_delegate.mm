// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/mac/notification_center_delegate.h"

#include <string>

#include "base/logging.h"
#include "electron/mas.h"
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

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notif
         withCompletionHandler:
             (void (^)(UNNotificationPresentationOptions options))
                 completionHandler {
  // Display notifications when app is in the foreground
  completionHandler(UNNotificationPresentationOptionList |
                    UNNotificationPresentationOptionBanner |
                    UNNotificationPresentationOptionSound);
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
    didReceiveNotificationResponse:(UNNotificationResponse*)response
             withCompletionHandler:(void (^)())completionHandler {
  auto* notification =
      presenter_->GetNotification(response.notification.request);

  if (electron::debug_notifications) {
    LOG(INFO) << "Notification activated ("
              << [response.notification.request.identifier UTF8String] << ")";
  }

  if (notification) {
    NSString* categoryIdentifier =
        response.notification.request.content.categoryIdentifier;
    NSString* actionIdentifier = response.actionIdentifier;
    if ([actionIdentifier
            isEqualToString:UNNotificationDefaultActionIdentifier]) {
      notification->NotificationClicked();
    } else if ([actionIdentifier
                   isEqualToString:UNNotificationDismissActionIdentifier]) {
      notification->NotificationDismissed();
    } else if ([categoryIdentifier hasPrefix:@"CATEGORY_"]) {
      if ([actionIdentifier isEqualToString:@"REPLY_ACTION"]) {
        if ([response isKindOfClass:[UNTextInputNotificationResponse class]]) {
          NSString* userText =
              [(UNTextInputNotificationResponse*)response userText];
          notification->NotificationReplied([userText UTF8String]);
        }
      } else if ([actionIdentifier hasPrefix:@"ACTION_"]) {
        NSString* actionIndexString =
            [actionIdentifier substringFromIndex:[@"ACTION_" length]];
        int actionIndex = static_cast<int>(actionIndexString.integerValue);
        notification->NotificationActivated(actionIndex);
      } else if ([actionIdentifier isEqualToString:@"CLOSE_ACTION"]) {
        notification->NotificationDismissed();
      }
    }
  } else {
    if (electron::debug_notifications) {
      LOG(INFO) << "Could not find notification for "
                << [response.notification.request.identifier UTF8String];
    }
  }

  completionHandler();
}

@end
