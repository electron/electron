// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "browser/atom_application_delegate_mac.h"

#include "base/strings/sys_string_conversions.h"
#import "browser/atom_application_mac.h"
#include "browser/browser.h"

@implementation AtomApplicationDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  // Trap the quit message to handleQuitEvent.
  NSAppleEventManager* em = [NSAppleEventManager sharedAppleEventManager];
  [em setEventHandler:self
          andSelector:@selector(handleQuitEvent:withReplyEvent:)
        forEventClass:kCoreEventClass
           andEventID:kAEQuitApplication];

  atom::Browser::Get()->DidFinishLaunching();
}

- (BOOL)application:(NSApplication*)sender
           openFile:(NSString*)filename {
  std::string filename_str(base::SysNSStringToUTF8(filename));
  return atom::Browser::Get()->OpenFile(filename_str) ? YES : NO;
}

- (void)handleQuitEvent:(NSAppleEventDescriptor*)event
         withReplyEvent:(NSAppleEventDescriptor*)replyEvent {
  [[AtomApplication sharedApplication] closeAllWindows:self];
}

@end
