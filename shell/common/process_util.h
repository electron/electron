// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_PROCESS_UTIL_H_
#define ELECTRON_SHELL_COMMON_PROCESS_UTIL_H_

#include <string>
#include <string_view>

namespace electron {

std::string GetProcessType();

bool IsBrowserProcess();
bool IsRendererProcess();
bool IsUtilityProcess();
bool IsZygoteProcess();

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_PROCESS_UTIL_H_
