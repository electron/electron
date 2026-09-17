// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CONVERTED_VALUE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CONVERTED_VALUE_H_

#include <optional>
#include <utility>

#include "gin/converter.h"
#include "v8/include/cppgc/visitor.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-traced-handle.h"

namespace gin_helper {

// A property backing store for cppgc-managed wrappables: keeps the JS value
// the script assigned (so the getter returns the identical object) together
// with its conversion to T, done once at assignment.
//
// Must be a field of a traced object; call Trace() from the owner's Trace().
template <typename T>
class ConvertedValue {
 public:
  ConvertedValue() = default;
  ConvertedValue(const ConvertedValue&) = delete;
  ConvertedValue& operator=(const ConvertedValue&) = delete;

  // Converts and stores |value|; null/undefined clears. Returns false and
  // leaves the holder unchanged if |value| does not convert to T.
  bool Set(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    if (value.IsEmpty() || value->IsNullOrUndefined()) {
      Reset();
      return true;
    }
    T converted;
    if (!gin::ConvertFromV8(isolate, value, &converted))
      return false;
    value_.Reset(isolate, value);
    converted_.emplace(std::move(converted));
    return true;
  }

  void Reset() {
    value_.Reset();
    converted_.reset();
  }

  bool IsEmpty() const { return !converted_.has_value(); }

  // The value as assigned, or |fallback| when unset.
  v8::Local<v8::Value> GetV8(v8::Isolate* isolate,
                             v8::Local<v8::Value> fallback) const {
    return value_.IsEmpty() ? fallback : value_.Get(isolate);
  }

  // The converted value; null when unset.
  const T* Get() const { return converted_ ? &*converted_ : nullptr; }

  // Converts the stored value again, for objects the script mutates in place.
  // Keeps the previous conversion if it no longer converts.
  const T* Refresh(v8::Isolate* isolate) {
    if (!value_.IsEmpty()) {
      v8::HandleScope handle_scope(isolate);
      T converted;
      if (gin::ConvertFromV8(isolate, value_.Get(isolate), &converted))
        converted_.emplace(std::move(converted));
    }
    return Get();
  }

  void Trace(cppgc::Visitor* visitor) const { visitor->Trace(value_); }

 private:
  v8::TracedReference<v8::Value> value_;
  std::optional<T> converted_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CONVERTED_VALUE_H_
