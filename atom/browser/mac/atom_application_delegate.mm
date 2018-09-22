// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/mac/atom_application_delegate.h"

#include "atom/browser/browser.h"
#import "atom/browser/mac/atom_application.h"
#include "atom/browser/mac/dict_util.h"
#include "base/allocator/allocator_shim.h"
#include "base/allocator/buildflags.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_objc_class_swizzler.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"

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
                 whichAttributes:(long long)attributes
               completionHandler:(void (^)(void))block {
  // The allocator flag is per-process, so other threads may temporarily
  // not have fatal OOM occur while this method executes, but it is better
  // than crashing when using IME.
  base::allocator::SetCallNewHandlerOnMallocFailure(false);
  g_swizzle_imk_input_session->GetOriginalImplementation()(self, _cmd, range,
                                                           attributes, block);
  base::allocator::SetCallNewHandlerOnMallocFailure(true);
}
@end
#endif  // BUILDFLAG(USE_ALLOCATOR_SHIM)

@implementation AtomApplicationDelegate

- (void)setApplicationDockMenu:(atom::AtomMenuModel*)model {
  menu_controller_.reset([[AtomMenuController alloc] initWithModel:model
                                             useDefaultAccelerator:NO]);
}

- (void)applicationWillFinishLaunching:(NSNotification*)notify {
  // Don't add the "Enter Full Screen" menu item automatically.
  [[NSUserDefaults standardUserDefaults]
      setBool:NO
       forKey:@"NSFullScreenMenuItemEverywhere"];

  atom::Browser::Get()->WillFinishLaunching();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  NSUserNotification* user_notification =
      [notify userInfo][(id) @"NSApplicationLaunchUserNotificationKey"];

  if (user_notification.userInfo != nil) {
    std::unique_ptr<base::DictionaryValue> launch_info =
        atom::NSDictionaryToDictionaryValue(user_notification.userInfo);
    atom::Browser::Get()->DidFinishLaunching(*launch_info);
  } else {
    atom::Browser::Get()->DidFinishLaunching(base::DictionaryValue());
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

- (NSMenu*)applicationDockMenu:(NSApplication*)sender {
  if (menu_controller_)
    return [menu_controller_ menu];
  else
    return nil;
}

- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename {
  std::string filename_str(base::SysNSStringToUTF8(filename));
  return atom::Browser::Get()->OpenFile(filename_str) ? YES : NO;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication
                    hasVisibleWindows:(BOOL)flag {
  atom::Browser* browser = atom::Browser::Get();
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
              restorationHandler API_AVAILABLE(macosx(10.10)) {
  std::string activity_type(base::SysNSStringToUTF8(userActivity.activityType));
  std::unique_ptr<base::DictionaryValue> user_info =
      atom::NSDictionaryToDictionaryValue(userActivity.userInfo);
  if (!user_info)
    return NO;

  atom::Browser* browser = atom::Browser::Get();
  return browser->ContinueUserActivity(activity_type, *user_info) ? YES : NO;
}

- (BOOL)application:(NSApplication*)application
    willContinueUserActivityWithType:(NSString*)userActivityType {
  std::string activity_type(base::SysNSStringToUTF8(userActivityType));

  atom::Browser* browser = atom::Browser::Get();
  return browser->WillContinueUserActivity(activity_type) ? YES : NO;
}

- (void)application:(NSApplication*)application
    didFailToContinueUserActivityWithType:(NSString*)userActivityType
                                    error:(NSError*)error {
  std::string activity_type(base::SysNSStringToUTF8(userActivityType));
  std::string error_message(
      base::SysNSStringToUTF8([error localizedDescription]));

  atom::Browser* browser = atom::Browser::Get();
  browser->DidFailToContinueUserActivity(activity_type, error_message);
}

- (IBAction)newWindowForTab:(id)sender {
  atom::Browser::Get()->NewWindowForTab();
}

@end
