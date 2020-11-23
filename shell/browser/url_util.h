// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_URL_UTIL_H_
#define SHELL_BROWSER_URL_UTIL_H_

#include <set>
#include <string>

#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

namespace electron {

bool MatchesFilterCondition(const GURL original_request,
                            const std::set<URLPattern>& patterns);
std::set<URLPattern> ParseURLPatterns(
    const std::set<std::string>& filter_patterns,
    std::string* error);

}  // namespace electron

#endif  // SHELL_BROWSER_URL_UTIL_H_
