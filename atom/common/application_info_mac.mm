// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/common/application_info.h"

#import "atom/common/mac/main_application_bundle.h"
#import "base/mac/foundation_util.h"
#import "base/strings/sys_string_conversions.h"

namespace atom {

namespace {

std::string ApplicationInfoDictionaryValue(NSString* key) {
  return base::SysNSStringToUTF8(
      [MainApplicationBundle().infoDictionary objectForKey:key]);
}

std::string ApplicationInfoDictionaryValue(CFStringRef key) {
  return ApplicationInfoDictionaryValue(base::mac::CFToNSCast(key));
}

}  // namespace

std::string GetApplicationName() {
  return ApplicationInfoDictionaryValue(kCFBundleNameKey);
}

std::string GetApplicationVersion() {
  return ApplicationInfoDictionaryValue(@"CFBundleShortVersionString");
}

}  // namespace atom
