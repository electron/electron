// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_OBJECT_BUILDER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_OBJECT_BUILDER_H_

#include <cstddef>

#include "base/memory/raw_ptr.h"
#include "gin/converter.h"
#include "shell/common/gin_helper/interned_strings.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-object.h"

namespace gin_helper {

// Builds a plain object from a fixed set of fields, as own data properties
// with per-isolate cached keys. About half the cost of gin_helper::Dictionary
// (no prototype walk, no re-interning of the key on every call), and the
// result keeps the same shape and prototype as an object literal would.
//
//   return gin_helper::ObjectBuilder(isolate)
//       .Set("x", rect.x())
//       .Set("y", rect.y())
//       .Build();
class ObjectBuilder {
 public:
  explicit ObjectBuilder(v8::Isolate* isolate);
  ~ObjectBuilder();

  ObjectBuilder(const ObjectBuilder&) = delete;
  ObjectBuilder& operator=(const ObjectBuilder&) = delete;

  // |key| must be a string literal; see InternedString.
  template <size_t N, typename T>
  ObjectBuilder& Set(const char (&key)[N], const T& value) {
    v8::Local<v8::Value> v8_value;
    if (gin::TryConvertToV8(isolate_, value, &v8_value)) [[likely]] {
      object_
          ->CreateDataProperty(context_, InternedString(isolate_, key),
                               v8_value)
          .Check();
    }
    return *this;
  }

  [[nodiscard]] v8::Local<v8::Object> Build();

 private:
  const raw_ptr<v8::Isolate> isolate_;
  v8::Local<v8::Context> context_;
  v8::Local<v8::Object> object_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_OBJECT_BUILDER_H_
