// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_system_preferences.h"

#import <Cocoa/Cocoa.h>

namespace atom {

namespace api {

#if defined(OS_MACOSX)
bool SystemPreferences::IsDarkMode() {
  NSString* mode = [[NSUserDefaults standardUserDefaults]
      stringForKey:@"AppleInterfaceStyle"];
  return [mode isEqualToString:@"Dark"];
}
#endif

}  // namespace api

}  // namespace atom
