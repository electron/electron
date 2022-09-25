// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/browser/mac/electron_application_delegate.h"

#include <memory>
#include <string>

#include <sstream>

#include "base/allocator/buildflags.h"
#include "base/allocator/partition_allocator/shim/allocator_shim.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_objc_class_swizzler.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "shell/browser/api/electron_api_push_notifications.h"
#include "shell/browser/browser.h"
#include "shell/browser/mac/dict_util.h"
#import "shell/browser/mac/electron_application.h"

#import <UserNotifications/UserNotifications.h>
// SAP-16595 : Refine cold start procedure in macOS
#include "shell/browser/api/electron_api_app.h"

static NSDictionary* UNNotificationResponseToNSDictionary(
    UNNotificationResponse* response) API_AVAILABLE(macosx(10.14)) {
  // [response isKindOfClass:[UNNotificationResponse class]]
  if (![response respondsToSelector:@selector(actionIdentifier)] ||
      ![response respondsToSelector:@selector(notification)]) {
    return nil;
  }

  NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
  result[@"actionIdentifier"] = response.actionIdentifier;
  result[@"date"] = @(response.notification.date.timeIntervalSince1970);
  result[@"identifier"] = response.notification.request.identifier;
  result[@"userInfo"] = response.notification.request.content.userInfo;

  // [response isKindOfClass:[UNTextInputNotificationResponse class]]
  if ([response respondsToSelector:@selector(userText)]) {
    result[@"userText"] =
        static_cast<UNTextInputNotificationResponse*>(response).userText;
  }

  return result;
}

@implementation ElectronApplicationDelegate

- (void)setApplicationDockMenu:(electron::ElectronMenuModel*)model {
  menu_controller_.reset([[ElectronMenuController alloc] initWithModel:model
                                                 useDefaultAccelerator:NO]);
}

- (void)willPowerOff:(NSNotification*)notify {
  [[AtomApplication sharedApplication] willPowerOff:notify];
}

- (void)applicationWillFinishLaunching:(NSNotification*)notify {
  // Don't add the "Enter Full Screen" menu item automatically.
  [[NSUserDefaults standardUserDefaults]
      setBool:NO
       forKey:@"NSFullScreenMenuItemEverywhere"];

  [[[NSWorkspace sharedWorkspace] notificationCenter]
      addObserver:self
         selector:@selector(willPowerOff:)
             name:NSWorkspaceWillPowerOffNotification
           object:nil];

  electron::Browser::Get()->WillFinishLaunching();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  NSObject* user_notification =
      [notify userInfo][NSApplicationLaunchUserNotificationKey];
  NSDictionary* notification_info = nil;

  if (user_notification) {
    if ([user_notification isKindOfClass:[NSUserNotification class]]) {
      notification_info =
          [static_cast<NSUserNotification*>(user_notification) userInfo];
    } else if (@available(macOS 10.14, *)) {
      notification_info = UNNotificationResponseToNSDictionary(
          static_cast<UNNotificationResponse*>(user_notification));
    }
    auto* app = electron::api::App::Get();
    if (app) {
      NSUserNotification* notif =
          static_cast<NSUserNotification*>(user_notification);
      // SAP-16595 : Refine cold start procedure in macOS
      const char* action =
          notif.additionalActivationAction == nil
              ? nullptr
              : [notif.additionalActivationAction.identifier UTF8String];
      const char* reply =
          notif.response == nil ? nullptr : [notif.response.string UTF8String];

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
  }

  electron::Browser::Get()->DidFinishLaunching(
      electron::NSDictionaryToValue(notification_info));
}

- (void)applicationDidBecomeActive:(NSNotification*)notification {
  electron::Browser::Get()->DidBecomeActive();
}

- (void)applicationDidResignActive:(NSNotification*)notification {
  electron::Browser::Get()->DidResignActive();
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender {
  if (menu_controller_)
    return [menu_controller_ menu];
  else
    return nil;
}

- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename {
  std::string filename_str(base::SysNSStringToUTF8(filename));
  return electron::Browser::Get()->OpenFile(filename_str) ? YES : NO;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication
                    hasVisibleWindows:(BOOL)flag {
  electron::Browser* browser = electron::Browser::Get();
  browser->Activate(static_cast<bool>(flag));
  return flag;
}

- (BOOL)application:(NSApplication*)sender
    continueUserActivity:(NSUserActivity*)userActivity
      restorationHandler:
#ifdef MAC_OS_X_VERSION_10_14
          (void (^)(NSArray<id<NSUserActivityRestoring>>* restorableObjects))
#else
          (void (^)(NSArray* restorableObjects))
#endif
              restorationHandler {
  std::string activity_type(base::SysNSStringToUTF8(userActivity.activityType));
  NSURL* url = userActivity.webpageURL;
  NSDictionary* details = url ? @{@"webpageURL" : [url absoluteString]} : @{};
  if (!userActivity.userInfo)
    return NO;

  electron::Browser* browser = electron::Browser::Get();
  return browser->ContinueUserActivity(
             activity_type,
             electron::NSDictionaryToValue(userActivity.userInfo),
             electron::NSDictionaryToValue(details))
             ? YES
             : NO;
}

- (BOOL)application:(NSApplication*)application
    willContinueUserActivityWithType:(NSString*)userActivityType {
  std::string activity_type(base::SysNSStringToUTF8(userActivityType));

  electron::Browser* browser = electron::Browser::Get();
  return browser->WillContinueUserActivity(activity_type) ? YES : NO;
}

- (void)application:(NSApplication*)application
    didFailToContinueUserActivityWithType:(NSString*)userActivityType
                                    error:(NSError*)error {
  std::string activity_type(base::SysNSStringToUTF8(userActivityType));
  std::string error_message(
      base::SysNSStringToUTF8([error localizedDescription]));

  electron::Browser* browser = electron::Browser::Get();
  browser->DidFailToContinueUserActivity(activity_type, error_message);
}

- (IBAction)newWindowForTab:(id)sender {
  electron::Browser::Get()->NewWindowForTab();
}

- (void)application:(NSApplication*)application
    didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken {
  // https://stackoverflow.com/a/16411517
  const char* token_data = static_cast<const char*>([deviceToken bytes]);
  NSMutableString* token_string = [NSMutableString string];
  for (NSUInteger i = 0; i < [deviceToken length]; i++) {
    [token_string appendFormat:@"%02.2hhX", token_data[i]];
  }
  // Resolve outstanding APNS promises created during registration attempts
  electron::api::PushNotifications* push_notifications =
      electron::api::PushNotifications::Get();
  if (push_notifications) {
    push_notifications->ResolveAPNSPromiseSetWithToken(
        base::SysNSStringToUTF8(token_string));
  }
}

- (void)application:(NSApplication*)application
    didFailToRegisterForRemoteNotificationsWithError:(NSError*)error {
  std::string error_message(base::SysNSStringToUTF8(
      [NSString stringWithFormat:@"%ld %@ %@", [error code], [error domain],
                                 [error userInfo]]));
  electron::api::PushNotifications* push_notifications =
      electron::api::PushNotifications::Get();
  if (push_notifications) {
    push_notifications->RejectAPNSPromiseSetWithError(error_message);
  }
}

- (void)application:(NSApplication*)application
    didReceiveRemoteNotification:(NSDictionary*)userInfo {
  electron::api::PushNotifications* push_notifications =
      electron::api::PushNotifications::Get();
  if (push_notifications) {
    electron::api::PushNotifications::Get()->OnDidReceiveAPNSNotification(
        electron::NSDictionaryToValue(userInfo));
  }
}

@end
