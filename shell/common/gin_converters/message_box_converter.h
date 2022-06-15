// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_MESSAGE_BOX_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_MESSAGE_BOX_CONVERTER_H_

#include "gin/converter.h"
#include "shell/browser/ui/message_box.h"

namespace gin {

template <>
struct Converter<electron::MessageBoxSettings> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::MessageBoxSettings* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_MESSAGE_BOX_CONVERTER_H_
