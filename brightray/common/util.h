// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_COMMON_UTIL_H_
#define BRIGHTRAY_COMMON_UTIL_H_

#include <string>

namespace util {

#if defined(OS_LINUX)
// Returns a success flag.
// Unlike libgtkui, does *not* use "chromium-browser.desktop" as a fallback.
bool GetDesktopName(std::string* setme);
#endif

}  // namespace util

#endif  // BRIGHTRAY_COMMON_UTIL_H_
