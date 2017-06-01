// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_paths_internal.h"
#import "base/mac/foundation_util.h"

namespace asar {

std::string GetAsarChecksums() {
  NSBundle* bundle = chrome::OuterAppBundle();
  if (!bundle) {
    return "";
  }

  NSString *checksums = [bundle objectForInfoDictionaryKey:@"AsarChecksums"];
  return checksums ? std::string([checksums UTF8String]) : "";
}

}