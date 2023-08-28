// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_

#include "gin/converter.h"

namespace extensions {
class Extension;
}

namespace gin {

template <>
struct Converter<const extensions::Extension*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const extensions::Extension* val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_
