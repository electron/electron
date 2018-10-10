// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include "atom/browser/mac/atom_application_delegate.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace atom {

void AtomBrowserMainParts::PreMainMessageLoopStart() {
  // Set our own application delegate.
  AtomApplicationDelegate* delegate = [[AtomApplicationDelegate alloc] init];
  [NSApp setDelegate:delegate];

  brightray::BrowserMainParts::PreMainMessageLoopStart();

  // Prevent Cocoa from turning command-line arguments into
  // |-application:openFiles:|, since we already handle them directly.
  [[NSUserDefaults standardUserDefaults]
      setObject:@"NO"
         forKey:@"NSTreatUnknownArgumentsAsOpen"];
}

void AtomBrowserMainParts::FreeAppDelegate() {
  [[NSApp delegate] release];
  [NSApp setDelegate:nil];
}

}  // namespace atom
