// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/cached_string.h"

#include "gin/converter.h"
#include "v8/include/cppgc/visitor.h"
#include "v8/include/v8-cppgc.h"

namespace gin_helper {

CachedString::CachedString() = default;
CachedString::~CachedString() = default;

v8::Local<v8::String> CachedString::Get(v8::Isolate* isolate,
                                        std::string_view value) {
  if (handle_.IsEmpty() || value_ != value) {
    value_ = std::string(value);
    handle_.Reset(isolate, gin::StringToV8(isolate, value_));
  }
  return handle_.Get(isolate);
}

void CachedString::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(handle_);
}

}  // namespace gin_helper
