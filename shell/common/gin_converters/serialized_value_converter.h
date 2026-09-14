// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERIALIZED_VALUE_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERIALIZED_VALUE_CONVERTER_H_

#include "gin/converter.h"
#include "shell/common/serialized_value.h"

namespace gin {

template <>
struct Converter<electron::SerializedValue> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const electron::SerializedValue& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::SerializedValue* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERIALIZED_VALUE_CONVERTER_H_
