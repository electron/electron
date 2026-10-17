// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_INTERNED_STRINGS_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_INTERNED_STRINGS_H_

#include <cstddef>
#include <string_view>

#include "v8/include/v8-forward.h"

namespace gin_helper {

// Returns an internalized v8::String for a string literal, cached per isolate
// (about 3 ns instead of 14 for gin::StringToSymbol). The cache is keyed by
// the literal's address, so only pass literals; the array-reference parameter
// rules out std::string, string_view and const char*.
namespace internal {
v8::Local<v8::String> InternedStringImpl(v8::Isolate* isolate,
                                         std::string_view literal);
}  // namespace internal

template <size_t N>
v8::Local<v8::String> InternedString(v8::Isolate* isolate,
                                     const char (&literal)[N]) {
  return internal::InternedStringImpl(isolate,
                                      std::string_view{literal, N - 1});
}

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_INTERNED_STRINGS_H_
