// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CACHED_STRING_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CACHED_STRING_H_

#include <string>
#include <string_view>

#include "v8/include/v8-forward.h"
#include "v8/include/v8-traced-handle.h"

namespace cppgc {
class Visitor;
}

namespace gin_helper {

// Keeps the V8 string built from a value and hands the same one back until the
// value it was built from changes, for getters that answer with a string which
// almost never does (a name, a version, a path). Meant to be a member of a
// cppgc-managed object, which must trace it.
class CachedString {
 public:
  CachedString();
  ~CachedString();
  CachedString(const CachedString&) = delete;
  CachedString& operator=(const CachedString&) = delete;

  v8::Local<v8::String> Get(v8::Isolate* isolate, std::string_view value);

  void Trace(cppgc::Visitor* visitor) const;

 private:
  std::string value_;
  v8::TracedReference<v8::String> handle_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CACHED_STRING_H_
