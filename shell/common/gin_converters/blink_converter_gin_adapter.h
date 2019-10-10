// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_GIN_ADAPTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_GIN_ADAPTER_H_

#include "gin/converter.h"
#include "shell/common/native_mate_converters/blink_converter.h"

// TODO(zcbenz): Move the implementations from native_mate_converters to here.

namespace gin {

template <>
struct Converter<blink::CloneableMessage> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::CloneableMessage* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::CloneableMessage& val) {
    return mate::ConvertToV8(isolate, val);
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_GIN_ADAPTER_H_
