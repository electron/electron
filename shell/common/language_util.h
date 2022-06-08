// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_LANGUAGE_UTIL_H_
#define ELECTRON_SHELL_COMMON_LANGUAGE_UTIL_H_

#include <string>
#include <vector>

namespace electron {

// Return a list of user preferred languages from OS. The list doesn't include
// overrides from command line arguments.
std::vector<std::string> GetPreferredLanguages();

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_LANGUAGE_UTIL_H_
