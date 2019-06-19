// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_MESSAGE_BOX_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_MESSAGE_BOX_CONVERTER_H_

#include "native_mate/converter.h"
#include "shell/browser/ui/message_box.h"

namespace mate {

template <>
struct Converter<atom::MessageBoxSettings> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::MessageBoxSettings* out);
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_MESSAGE_BOX_CONVERTER_H_
