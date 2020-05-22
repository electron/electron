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

@implementation ElectronApplicationDelegate {
 @private
  bool isFirstActivation_;
}

- (void)setApplicationDockMenu:(electron::ElectronMenuModel*)model {
  menu_controller_.reset([[ElectronMenuController alloc] initWithModel:model
                                                 useDefaultAccelerator:NO]);
}

- (void)applicationWillFinishLaunching:(NSNotification*)notify {
  // Don't add the "Enter Full Screen" menu item automatically.
  [[NSUserDefaults standardUserDefaults]
      setBool:NO
       forKey:@"NSFullScreenMenuItemEverywhere"];

  electron::Browser::Get()->WillFinishLaunching();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  NSUserNotification* user_notification =
      [notify userInfo][(id) @"NSApplicationLaunchUserNotificationKey"];

  if (user_notification.userInfo) {
    electron::Browser::Get()->DidFinishLaunching(
        electron::NSDictionaryToDictionaryValue(user_notification.userInfo));
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

  isFirstActivation_ = true;
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

- (void)applicationDidBecomeActive:(NSNotification*)notify {
  // Originally `applicationShouldHandleReopen:hasVisibleWindows:` was used to
  // emit Activate event. But the message is sent only when application is
  // activated via Dock or Finder ignoring the App Switcher (cmd+tab).
  //
  // Using `applicationDidBecomeActive' is more reliable but to maintain
  // compatibility with previous implementation we ignore activation
  // immediately after the application launch, and compute the
  // hasVisibleWindows on our own.
  //
  // Details in https://github.com/electron/electron/pull/23727.

  if (isFirstActivation_) {
    isFirstActivation_ = false;
    return;
  }

  NSApplication* app = notify.object;
  bool hasVisibleWindows = false;

  for (NSWindow* win in app.windows) {
    if (win.isVisible || win.miniaturized) {
      hasVisibleWindows = true;
      break;
    }
  }

  electron::Browser* browser = electron::Browser::Get();
  browser->Activate(hasVisibleWindows);
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
  if (!userActivity.userInfo)
    return NO;

  electron::Browser* browser = electron::Browser::Get();
  return browser->ContinueUserActivity(
             activity_type,
             electron::NSDictionaryToDictionaryValue(userActivity.userInfo))
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

@end
