// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_COMMON_PLATFORM_UTIL_H_
#define BRIGHTRAY_COMMON_PLATFORM_UTIL_H_

#include <string>

namespace brightray {

namespace platform_util {

#if defined(OS_LINUX)
// Returns a success flag.
// Unlike libgtkui, does *not* use "chromium-browser.desktop" as a fallback.
bool GetDesktopName(std::string* setme);
#endif

}  // namespace platform_util

}  // namespace brightray

#endif  // BRIGHTRAY_COMMON_PLATFORM_UTIL_H_
