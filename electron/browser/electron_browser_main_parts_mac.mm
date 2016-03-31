// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/browser/electron_browser_main_parts.h"

#include "electron/browser/mac/electron_application.h"
#include "electron/browser/mac/electron_application_delegate.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace electron {

void ElectronBrowserMainParts::PreMainMessageLoopStart() {
  // Force the NSApplication subclass to be used.
  [ElectronApplication sharedApplication];

  // Set our own application delegate.
  ElectronApplicationDelegate* delegate = [[ElectronApplicationDelegate alloc] init];
  [NSApp setDelegate:(id<NSFileManagerDelegate>)delegate];

  brightray::BrowserMainParts::PreMainMessageLoopStart();

  // Prevent Cocoa from turning command-line arguments into
  // |-application:openFiles:|, since we already handle them directly.
  [[NSUserDefaults standardUserDefaults]
      setObject:@"NO" forKey:@"NSTreatUnknownArgumentsAsOpen"];
}

void ElectronBrowserMainParts::FreeAppDelegate() {
  [[NSApp delegate] release];
  [NSApp setDelegate:nil];
}

}  // namespace electron
