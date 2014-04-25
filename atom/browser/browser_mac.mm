// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#import "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#import "atom/browser/mac/atom_application.h"

namespace atom {

void Browser::Focus() {
  [[AtomApplication sharedApplication] activateIgnoringOtherApps:YES];
}

std::string Browser::GetExecutableFileVersion() const {
  NSDictionary* infoDictionary = base::mac::OuterBundle().infoDictionary;
  NSString *version = [infoDictionary objectForKey:@"CFBundleVersion"];
  return base::SysNSStringToUTF8(version);
}

std::string Browser::GetExecutableFileProductName() const {
  NSDictionary* infoDictionary = base::mac::OuterBundle().infoDictionary;
  NSString *version = [infoDictionary objectForKey:@"CFBundleName"];
  return base::SysNSStringToUTF8(version);
}

int Browser::DockBounce(BounceType type) {
  return [[AtomApplication sharedApplication] requestUserAttention:type];
}

void Browser::DockCancelBounce(int rid) {
  [[AtomApplication sharedApplication] cancelUserAttentionRequest:rid];
}

void Browser::DockSetBadgeText(const std::string& label) {
  NSDockTile *tile = [[AtomApplication sharedApplication] dockTile];
  [tile setBadgeLabel:base::SysUTF8ToNSString(label)];
}

std::string Browser::DockGetBadgeText() {
  NSDockTile *tile = [[AtomApplication sharedApplication] dockTile];
  return base::SysNSStringToUTF8([tile badgeLabel]);
}

}  // namespace atom
