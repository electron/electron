// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#import "atom/browser/mac/atom_application.h"
#import "atom/browser/mac/atom_application_delegate.h"
#include "base/files/file_path.h"
#import "base/mac/foundation_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#import "vendor/brightray/common/mac/main_application_bundle.h"

namespace atom {

std::string GetApplicationName() {
  std::string name = brightray::MainApplicationBundlePath().BaseName().AsUTF8Unsafe();
  return name.substr(0, name.length() - 4/*.app*/);
}

void AtomBrowserMainParts::PreMainMessageLoopStart() {
  // Initialize locale setting.
  l10n_util::OverrideLocaleWithCocoaLocale();

  // Force the NSApplication subclass to be used.
  NSApplication* application = [AtomApplication sharedApplication];

  AtomApplicationDelegate* delegate = [[AtomApplicationDelegate alloc] init];
  [NSApp setDelegate:(id<NSFileManagerDelegate>)delegate];

  base::FilePath frameworkPath = brightray::MainApplicationBundlePath()
      .Append("Contents")
      .Append("Frameworks")
      .Append(GetApplicationName() + " Framework.framework");
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
  [[NSApp delegate] release];
  [NSApp setDelegate:nil];
}

}  // namespace atom
