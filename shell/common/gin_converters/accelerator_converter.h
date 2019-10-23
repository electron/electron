// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_ACCELERATOR_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_ACCELERATOR_CONVERTER_H_

#include "gin/converter.h"

namespace ui {
class Accelerator;
}

namespace gin {

template <>
struct Converter<ui::Accelerator> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ui::Accelerator* out);
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_ACCELERATOR_CONVERTER_H_
