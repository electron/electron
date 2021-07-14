// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_browser_main_parts.h"

#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "services/device/public/cpp/geolocation/geolocation_manager_impl_mac.h"
#import "shell/browser/mac/electron_application.h"
#include "shell/browser/mac/electron_application_delegate.h"
#include "shell/common/electron_paths.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace electron {

void ElectronBrowserMainParts::PreCreateMainMessageLoop() {
  // Set our own application delegate.
  ElectronApplicationDelegate* delegate =
      [[ElectronApplicationDelegate alloc] init];
  [NSApp setDelegate:delegate];

  PreCreateMainMessageLoopCommon();

  // Prevent Cocoa from turning command-line arguments into
  // |-application:openFiles:|, since we already handle them directly.
  [[NSUserDefaults standardUserDefaults]
      setObject:@"NO"
         forKey:@"NSTreatUnknownArgumentsAsOpen"];

  geolocation_manager_ = device::GeolocationManagerImpl::Create();
}

void ElectronBrowserMainParts::FreeAppDelegate() {
  [[NSApp delegate] release];
  [NSApp setDelegate:nil];
}

void ElectronBrowserMainParts::RegisterURLHandler() {
  [[AtomApplication sharedApplication] registerURLHandler];
}

// Replicates NSApplicationMain, but doesn't start a run loop.
void ElectronBrowserMainParts::InitializeMainNib() {
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
