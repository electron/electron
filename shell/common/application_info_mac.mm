// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/application_info.h"

#include <string>

#include "base/apple/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "shell/common/mac/main_application_bundle.h"

namespace electron {

namespace {

std::string ApplicationInfoDictionaryValue(NSString* key) {
  return base::SysNSStringToUTF8(
      [MainApplicationBundle().infoDictionary objectForKey:key]);
}

std::string ApplicationInfoDictionaryValue(CFStringRef key) {
  NSString* key_ns = const_cast<NSString*>((__bridge const NSString*)(key));
  return ApplicationInfoDictionaryValue(key_ns);
}

}  // namespace

std::string GetApplicationName() {
  return ApplicationInfoDictionaryValue(kCFBundleNameKey);
}

std::string GetApplicationVersion() {
  return ApplicationInfoDictionaryValue(@"CFBundleShortVersionString");
}

}  // namespace electron
