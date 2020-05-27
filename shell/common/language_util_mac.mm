// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/language_util.h"

#import <Cocoa/Cocoa.h>
#include <string>
#include <vector>

#include "base/strings/sys_string_conversions.h"

namespace electron {

std::vector<std::string> GetPreferredLanguages() {
  __block std::vector<std::string> languages;
  [[NSLocale preferredLanguages]
      enumerateObjectsUsingBlock:^(NSString* language, NSUInteger i,
                                   BOOL* stop) {
        languages.push_back(base::SysNSStringToUTF8(language));
      }];
  return languages;
}

}  // namespace electron
