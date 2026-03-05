// Copyright (c) 2025 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LINUX_OZONE_UTIL_H_
#define ELECTRON_SHELL_BROWSER_LINUX_OZONE_UTIL_H_

namespace ozone_util {

bool IsX11();
bool IsWayland();

}  // namespace ozone_util

#endif  // ELECTRON_SHELL_BROWSER_LINUX_OZONE_UTIL_H_
