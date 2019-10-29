// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_browser_main_parts.h"

#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "shell/browser/atom_paths.h"
#import "shell/browser/mac/atom_application.h"
#include "shell/browser/mac/atom_application_delegate.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace electron {

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

void AtomBrowserMainParts::RegisterURLHandler() {
  [[AtomApplication sharedApplication] registerURLHandler];
}

// Replicates NSApplicationMain, but doesn't start a run loop.
void AtomBrowserMainParts::InitializeMainNib() {
  auto infoDictionary = base::mac::OuterBundle().infoDictionary;

  auto principalClass =
      NSClassFromString([infoDictionary objectForKey:@"NSPrincipalClass"]);
  auto application = [principalClass sharedApplication];

  NSString* mainNibName = [infoDictionary objectForKey:@"NSMainNibFile"];

  NSNib* mainNib;

  @try {
    mainNib = [[NSNib alloc] initWithNibNamed:mainNibName
                                       bundle:base::mac::FrameworkBundle()];
    // Handle failure of initWithNibNamed on SMB shares
    // TODO(codebytere): Remove when
    // https://bugs.chromium.org/p/chromium/issues/detail?id=932935 is fixed
  } @catch (NSException* exception) {
    NSString* nibPath =
        [NSString stringWithFormat:@"Resources/%@.nib", mainNibName];
    nibPath = [base::mac::FrameworkBundle().bundlePath
        stringByAppendingPathComponent:nibPath];

    NSData* data = [NSData dataWithContentsOfFile:nibPath];
    mainNib = [[NSNib alloc] initWithNibData:data
                                      bundle:base::mac::FrameworkBundle()];
  }

  [mainNib instantiateWithOwner:application topLevelObjects:nil];
  [mainNib release];
}

}  // namespace electron
