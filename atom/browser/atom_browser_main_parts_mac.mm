// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include "atom/browser/atom_paths.h"
#include "atom/browser/mac/atom_application_delegate.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace atom {

void AtomBrowserMainParts::PreMainMessageLoopStart() {
  // Set our own application delegate.
  AtomApplicationDelegate* delegate = [[AtomApplicationDelegate alloc] init];
  [NSApp setDelegate:delegate];

  PreMainMessageLoopStartCommon();

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

void AtomBrowserMainParts::OverrideAppLogsPath() {
  base::FilePath path;
  NSString* bundleName =
      [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
  NSString* logsPath =
      [NSString stringWithFormat:@"Library/Logs/%@", bundleName];
  NSString* libraryPath =
      [NSHomeDirectory() stringByAppendingPathComponent:logsPath];

  base::PathService::Override(DIR_APP_LOGS,
                              base::FilePath([libraryPath UTF8String]));
}

// Replicates NSApplicationMain, but doesn't start a run loop.
void AtomBrowserMainParts::InitializeMainNib() {
  auto infoDictionary = base::mac::OuterBundle().infoDictionary;

  auto principalClass =
      NSClassFromString([infoDictionary objectForKey:@"NSPrincipalClass"]);
  auto application = [principalClass sharedApplication];

  NSString* mainNibName = [infoDictionary objectForKey:@"NSMainNibFile"];
  auto mainNib = [[NSNib alloc] initWithNibNamed:mainNibName
                                          bundle:base::mac::FrameworkBundle()];
  [mainNib instantiateWithOwner:application topLevelObjects:nil];
  [mainNib release];
}

}  // namespace atom
