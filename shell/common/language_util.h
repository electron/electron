// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_LANGUAGE_UTIL_H_
#define SHELL_COMMON_LANGUAGE_UTIL_H_

#include <string>
#include <vector>

#include "base/strings/string16.h"

namespace electron {

// Return a list of user preferred languages from OS. The list doesn't include
// overrides from command line arguments.
std::vector<std::string> GetPreferredLanguages();

#if defined(OS_WIN)
bool GetPreferredLanguagesUsingGlobalization(
    std::vector<base::string16>* languages);
#endif

}  // namespace electron

#endif  // SHELL_COMMON_LANGUAGE_UTIL_H_
