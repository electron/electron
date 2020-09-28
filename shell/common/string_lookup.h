// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <algorithm>
#include <array>
#include <string>

#include "base/optional.h"
#include "base/strings/string_piece.h"

namespace electron {

/**
 * Efficient constexpr mapping of string/value pairs known at compile time.
 * Example code:
 *
 * // declaration
 * static constexpr StringLookup<int, 6> candidates {{{
 *   { "ford", 15 },
 *   { "jarrah", 16 },
 *   { "kwon", 42 }
 *   { "locke", 4 },
 *   { "reyes", 8 },
 *   { "shephard", 23 },
 * }}};
 * static_assert(candidates.IsSorted(), "please sort candidates");
 *
 * // usage
 * base::Optional<int> number = candidates.lookup("kwon");
 */
template <typename Value, size_t Size>
struct StringLookup {
  std::array<std::pair<base::StringPiece, Value>, Size> data;

  base::Optional<Value> lookup(base::StringPiece key) const {
    const auto compare = [](auto const& a, auto const& b) {
      return a.first < b;
    };
    const auto* it =
        std::lower_bound(std::cbegin(data), std::cend(data), key, compare);
    return it != std::cend(data) && key == it->first ? it->second
                                                     : base::Optional<Value>{};
  }

  constexpr bool IsSorted() const {
    for (std::size_t i = 0; i < data.size() - 1; ++i) {
      if (data[i].first > data[i + 1].first) {
        return false;
      }
    }
    return true;
  }
};

}  // namespace electron
