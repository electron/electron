// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_MESSAGE_BOX_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_MESSAGE_BOX_CONVERTER_H_

#include "native_mate/converter.h"
#include "shell/browser/ui/message_box.h"

namespace mate {

template <>
struct Converter<electron::MessageBoxSettings> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::MessageBoxSettings* out);
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_MESSAGE_BOX_CONVERTER_H_
