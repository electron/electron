// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/browser/mac/electron_application_delegate.h"

#include <memory>
#include <string>

#include "base/allocator/allocator_shim.h"
#include "base/allocator/buildflags.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_objc_class_swizzler.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "shell/browser/browser.h"
#include "shell/browser/mac/dict_util.h"
#import "shell/browser/mac/electron_application.h"

#import <UserNotifications/UserNotifications.h>

#if BUILDFLAG(USE_ALLOCATOR_SHIM)
// On macOS 10.12, the IME system attempts to allocate a 2^64 size buffer,
// which would typically cause an OOM crash. To avoid this, the problematic
// method is swizzled out and the make-OOM-fatal bit is disabled for the
// duration of the original call. https://crbug.com/654695
static base::mac::ScopedObjCClassSwizzler* g_swizzle_imk_input_session;
@interface OOMDisabledIMKInputSession : NSObject
@end
@implementation OOMDisabledIMKInputSession
- (void)_coreAttributesFromRange:(NSRange)range
                 whichAttributes:(long long)attributes  // NOLINT(runtime/int)
               completionHandler:(void (^)(void))block {
  // The allocator flag is per-process, so other threads may temporarily
  // not have fatal OOM occur while this method executes, but it is better
  // than crashing when using IME.
  base::allocator::SetCallNewHandlerOnMallocFailure(false);
  g_swizzle_imk_input_session->InvokeOriginal<
      void, NSRange, long long, void (^)(void)>(  // NOLINT(runtime/int)
      self, _cmd, range, attributes, block);
  base::allocator::SetCallNewHandlerOnMallocFailure(true);
}
@end
#endif  // BUILDFLAG(USE_ALLOCATOR_SHIM)

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
    [notify userInfo][(id) @"NSApplicationLaunchUserNotificationKey"];
  NSDictionary* notification_info = nil;

  if (user_notification) {
    if ([user_notification isKindOfClass:[NSUserNotification class]]) {
      notification_info =
        [(NSUserNotification *)user_notification userInfo];
    } else if (@available(macOS 10.14, *)) {
      if ([user_notification isKindOfClass:[UNNotificationResponse class]]) {
        notification_info = atom::UNNotificationResponseToNSDictionary(
          (UNNotificationResponse *)user_notification);
      }
    }
  }

  if (notification_info) {
    std::unique_ptr<base::DictionaryValue> launch_info =
        atom::NSDictionaryToDictionaryValue(notification_info);
    electron::Browser::Get()->DidFinishLaunching(*launch_info);
  } else {
    electron::Browser::Get()->DidFinishLaunching(base::DictionaryValue());
  }

#if BUILDFLAG(USE_ALLOCATOR_SHIM)
  // Disable fatal OOM to hack around an OS bug https://crbug.com/654695.
  if (base::mac::IsOS10_12()) {
    g_swizzle_imk_input_session = new base::mac::ScopedObjCClassSwizzler(
        NSClassFromString(@"IMKInputSession"),
        [OOMDisabledIMKInputSession class],
        @selector(_coreAttributesFromRange:whichAttributes:completionHandler:));
  }
#endif
}

- (void)applicationDidBecomeActive:(NSNotification*)notification {
  electron::Browser::Get()->DidBecomeActive();
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
             electron::NSDictionaryToDictionaryValue(userActivity.userInfo),
             electron::NSDictionaryToDictionaryValue(details))
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
  const char *tokenData = (const char *)[deviceToken bytes];
  NSMutableString *tokenString = [NSMutableString string];
  for (NSUInteger i = 0; i < [deviceToken length]; i++) {
    [tokenString appendFormat:@"%02.2hhX", tokenData[i]];
  }
  atom::Browser* browser = atom::Browser::Get();
  browser->DidRegisterForRemoteNotificationsWithDeviceToken(
    base::SysNSStringToUTF8(tokenString));
}

- (void)application:(NSApplication*)application
    didFailToRegisterForRemoteNotificationsWithError:(NSError*)error {
  std::string error_message(base::SysNSStringToUTF8(
    [NSString stringWithFormat:@"%ld %@ %@",
      [error code],
      [error domain],
      [error userInfo]]));
  atom::Browser* browser = atom::Browser::Get();
  browser->DidFailToRegisterForRemoteNotificationsWithError(error_message);
}

- (void)application:(NSApplication*)application
    didReceiveRemoteNotification:(NSDictionary*)userInfo {
  std::unique_ptr<base::DictionaryValue> user_info =
      atom::NSDictionaryToDictionaryValue(userInfo);
  atom::Browser* browser = atom::Browser::Get();
  browser->DidReceiveRemoteNotification(*user_info);
}

@end
