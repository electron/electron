// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include "base/files/file_path.h"
#import "base/mac/foundation_util.h"
#import "atom/browser/atom_application_mac.h"
#import "atom/browser/atom_application_delegate_mac.h"
#import "vendor/brightray/common/mac/main_application_bundle.h"

namespace atom {

void AtomBrowserMainParts::PreMainMessageLoopStart() {
  // Force the NSApplication subclass to be used.
  NSApplication* application = [AtomApplication sharedApplication];

  AtomApplicationDelegate* delegate = [AtomApplicationDelegate alloc];
  [NSApp setDelegate:delegate];

  base::FilePath frameworkPath = brightray::MainApplicationBundlePath()
      .Append("Contents")
      .Append("Frameworks")
      .Append("Atom Framework.framework");
  NSBundle* frameworkBundle = [NSBundle
       bundleWithPath:base::mac::FilePathToNSString(frameworkPath)];
  NSNib* mainNib = [[NSNib alloc] initWithNibNamed:@"MainMenu"
                                            bundle:frameworkBundle];
  [mainNib instantiateWithOwner:application topLevelObjects:nil];
  [mainNib release];

  // Prevent Cocoa from turning command-line arguments into
  // |-application:openFiles:|, since we already handle them directly.
  [[NSUserDefaults standardUserDefaults]
      setObject:@"NO" forKey:@"NSTreatUnknownArgumentsAsOpen"];
}

void AtomBrowserMainParts::PostDestroyThreads() {
  [[AtomApplication sharedApplication] setDelegate:nil];
}

}  // namespace atom
