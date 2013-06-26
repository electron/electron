// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "browser/atom_application_delegate_mac.h"

#include "base/strings/sys_string_conversions.h"
#import "browser/atom_application_mac.h"
#include "browser/browser.h"

@implementation AtomApplicationDelegate

- (void)applicationWillFinishLaunching:(NSNotification*)notify {
  atom::Browser::Get()->WillFinishLaunching();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  atom::Browser::Get()->DidFinishLaunching();
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
    return NSTerminateLater;
  }
}

@end
