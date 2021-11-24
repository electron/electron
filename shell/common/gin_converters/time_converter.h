// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_TIME_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_TIME_CONVERTER_H_

#include "gin/converter.h"

namespace base {
class Time;
}

namespace gin {

template <>
struct Converter<base::Time> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const base::Time& val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_TIME_CONVERTER_H_
