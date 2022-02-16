// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/mac/notification_center_delegate.h"

#include <string>

#include "base/callback_forward.h"
#include "base/logging.h"
#include "shell/browser/notifications/mac/cocoa_notification.h"
#include "shell/browser/notifications/mac/notification_presenter_mac.h"

#include "shell/browser/api/electron_api_app.h"

@implementation NotificationCenterDelegate {
  base::scoped_nsobject<NSXPCConnection> connection_;
}

- (NSXPCConnection*)connection {
  return connection_;
}

- (instancetype)initWithPresenter:
    (electron::NotificationPresenterMac*)presenter {
  self = [super init];
  if (!self)
    return nil;
  // making connection to the xpc AlertService
  NSString* bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
  bundleIdentifier =
      [bundleIdentifier stringByAppendingString:@".AlertService"];
  NSLog(@"bundleIdentifier : %@", bundleIdentifier);
  NSXPCConnection* connectionToService_ =
      [[[NSXPCConnection alloc] initWithServiceName:bundleIdentifier] retain];
  connectionToService_.remoteObjectInterface = [NSXPCInterface
      interfaceWithProtocol:@protocol(NSAlertDeliveryXPCProtocol)];
  connectionToService_.exportedInterface = [NSXPCInterface
      interfaceWithProtocol:@protocol(NSAlertResponceXPCProtocol)];
  connectionToService_.exportedObject = self;
  [connectionToService_ resume];
  NSLog(@"connectionToService_ : %@", connectionToService_);
  NSLog(@"connectionToService_.remoteObjectProxy : %@",
        connectionToService_.remoteObjectProxy);
  NSLog(@"connectionToService_.exportedObject    : %@",
        connectionToService_.exportedObject);
  connection_.reset(connectionToService_);
  presenter_ = presenter;
  return self;
}

- (void)didDeliverNotification:(NSUserNotification*)notif {
  NSLog(@"didDeliverNotification");
  dispatch_async(dispatch_get_main_queue(), ^{
    auto* notification = presenter_->GetNotification(notif);
    if (notification)
      notification->NotificationDisplayed();
  });
}

- (void)didActivateNotification:(NSUserNotification*)notif {
  BOOL isMainThread = [NSThread isMainThread];
  NSLog(@"didActivateNotification (is MainThread -> %@): %@",
        isMainThread ? @"YES" : @"NO", notif);
  dispatch_async(dispatch_get_main_queue(), ^{
    // SAP-14881 - Persistent notification on macOS
    // code bellow provides support for handling notifications from cold start
    auto* app = electron::api::App::Get();
    if (app) {
      const char* action =
          notif.additionalActivationAction == nil
              ? nullptr
              : [notif.additionalActivationAction.identifier UTF8String];
      const char* reply =
          notif.response == nil ? nullptr : [notif.response.string UTF8String];
      // SAP-15908 refine notification-activation for cold start in macOS
      if (!action && !reply) {  // click on toast body
        // in case if data exists forwarding it as invoked argument
        NSString* _data = notif.userInfo ? notif.userInfo[@"_data"] : @"";
        action = [_data UTF8String];
      }
      // SAP-15908 - Refine notification-activation for cold start in macOS
      std::stringstream stm;
      if (reply) {
        NSString* _replyKey =
            notif.userInfo ? notif.userInfo[@"_replyKey"] : @"";
        const char* key = [_replyKey UTF8String];
        stm << L"[";  // json array open brace
        stm << L"{";  // open brace for key-value pair
        stm << L"\"" << (key ? key : "") << L"\"";
        stm << L":";  // json delimeter (i.e colon) between key value
        stm << L"\"" << reply << L"\"";
        stm << L"}";  // close brace for key-value pair
        stm << L"]";  // json array close brace
      }
      app->Emit("notification-activation", [notif.identifier UTF8String],
                action ? action : "", reply ? 1 : 0, reply ? stm.str() : "");
    }

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
          notification->NotificationActivated(
              [notif additionalActivationAction]);
        }
      }
    }
  });
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
        didDeliverNotification:(NSUserNotification*)notification {
  [self didDeliverNotification:notification];
  // Clear pending notification
  presenter_->CloseNotificationWithId("![DELETE]");
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
       didActivateNotification:(NSUserNotification*)notification {
  [self didActivateNotification:notification];
  // Clear pending notification
  presenter_->CloseNotificationWithId("![DELETE]");
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification {
  // Display notifications even if the app is active.
  // SAP-20223 [N] TECH [MACOS] To support renotify on macOS
  NSDictionary* userInfo = notification.userInfo;
  NSString* shouldBePresented =
      userInfo && [userInfo valueForKey:@"_shouldBePresented"]
          ? userInfo[@"_shouldBePresented"]
          : @"YES";
  return [shouldBePresented isEqualToString:@"YES"] ? YES : NO;
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
