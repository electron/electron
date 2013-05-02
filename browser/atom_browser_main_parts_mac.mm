// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_main_parts.h"

#import "base/mac/bundle_locations.h"
#import "browser/atom_application.h"
#import "browser/atom_application_delegate.h"

namespace atom {

void AtomBrowserMainParts::PreMainMessageLoopStart() {
  // Force the NSApplication subclass to be used.
  NSApplication* application = [AtomApplication sharedApplication];

  AtomApplicationDelegate* delegate = [AtomApplicationDelegate alloc];
  [NSApp setDelegate:delegate];

  auto infoDictionary = base::mac::OuterBundle().infoDictionary;

  NSString *mainNibName = [infoDictionary objectForKey:@"NSMainNibFile"];
  auto mainNib = [[NSNib alloc] initWithNibNamed:mainNibName bundle:base::mac::FrameworkBundle()];
  [mainNib instantiateWithOwner:application topLevelObjects:nil];
  [mainNib release];
}

}  // namespace atom
