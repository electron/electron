// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <set>
#include <string>
#include <utility>

#include "shell/browser/url_util.h"

namespace electron {

bool MatchesFilterCondition(const GURL url,
                            const std::set<URLPattern>& patterns) {
  if (patterns.empty())
    return true;

  for (const auto& pattern : patterns) {
    if (pattern.MatchesURL(url))
      return true;
  }
  return false;
}

std::set<URLPattern> ParseURLPatterns(
    const std::set<std::string>& filter_patterns,
    std::string* error) {
  std::set<URLPattern> rv;

  for (const std::string& filter_pattern : filter_patterns) {
    URLPattern pattern(URLPattern::SCHEME_ALL);
    const URLPattern::ParseResult result =
        pattern.Parse(std::move(filter_pattern));
    if (result == URLPattern::ParseResult::kSuccess) {
      rv.insert(pattern);
    } else {
      const char* error_type = URLPattern::GetParseResultString(result);
      *error = "Invalid url pattern " + filter_pattern + ": " + error_type;
      break;
    }
  }

  return rv;
}

}  // namespace electron
