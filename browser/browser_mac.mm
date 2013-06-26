// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/browser.h"

#import "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#import "browser/atom_application_mac.h"

namespace atom {

void Browser::Terminate() {
  is_quiting_ = true;
  [[AtomApplication sharedApplication] terminate:nil];
}

void Browser::Focus() {
  [[AtomApplication sharedApplication] activateIgnoringOtherApps:YES];
}

std::string Browser::GetVersion() {
  NSDictionary* infoDictionary = base::mac::OuterBundle().infoDictionary;
  NSString *version = [infoDictionary objectForKey:@"CFBundleVersion"];
  return base::SysNSStringToUTF8(version);
}

void Browser::CancelQuit() {
  [[AtomApplication sharedApplication] replyToApplicationShouldTerminate:NO];
}

}  // namespace atom
