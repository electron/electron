// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/mac/atom_application_delegate.h"

#import "atom/browser/mac/atom_application.h"
#include "atom/browser/browser.h"
#include "atom/browser/mac/dict_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"

@implementation AtomApplicationDelegate

- (void)setApplicationDockMenu:(atom::AtomMenuModel*)model {
  menu_controller_.reset([[AtomMenuController alloc] initWithModel:model
                                             useDefaultAccelerator:NO]);
}

- (void)applicationWillFinishLaunching:(NSNotification*)notify {
  // Don't add the "Enter Full Screen" menu item automatically.
  [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];

  atom::Browser::Get()->WillFinishLaunching();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  atom::Browser::Get()->DidFinishLaunching();
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender {
  if (menu_controller_)
    return [menu_controller_ menu];
  else
    return nil;
}

- (BOOL)application:(NSApplication*)sender
           openFile:(NSString*)filename {
  std::string filename_str(base::SysNSStringToUTF8(filename));
  return atom::Browser::Get()->OpenFile(filename_str) ? YES : NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
  atom::Browser* browser = atom::Browser::Get();
  if (browser->is_quiting()) {
    return NSTerminateNow;
  } else {
    // System started termination.
    atom::Browser::Get()->Quit();
    return NSTerminateCancel;
  }
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication
                    hasVisibleWindows:(BOOL)flag {
  atom::Browser* browser = atom::Browser::Get();
  browser->Activate(static_cast<bool>(flag));
  return flag;
}

-  (BOOL)application:(NSApplication*)sender
continueUserActivity:(NSUserActivity*)userActivity
  restorationHandler:(void (^)(NSArray*restorableObjects))restorationHandler {
  std::string activity_type(base::SysNSStringToUTF8(userActivity.activityType));
  std::unique_ptr<base::DictionaryValue> user_info =
      atom::NSDictionaryToDictionaryValue(userActivity.userInfo);
  if (!user_info)
    return NO;

  atom::Browser* browser = atom::Browser::Get();
  return browser->ContinueUserActivity(activity_type, *user_info) ? YES : NO;
}

@end
