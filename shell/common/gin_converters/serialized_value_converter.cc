// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/serialized_value_converter.h"

#include "shell/common/v8_util.h"

namespace gin {

v8::Local<v8::Value> Converter<electron::SerializedValue>::ToV8(
    v8::Isolate* isolate,
    const electron::SerializedValue& in) {
  return electron::DeserializeV8Value(isolate, in);
}

bool Converter<electron::SerializedValue>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    electron::SerializedValue* out) {
  return electron::SerializeV8Value(isolate, val, out);
}

}  // namespace gin
