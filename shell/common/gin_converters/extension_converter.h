// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_

#include <string>

#include "gin/converter.h"

namespace extensions {
class Extension;
}  // namespace extensions

namespace electron {
namespace api {
struct ExtensionTabDetails;
}
}  // namespace electron

namespace gin {

template <>
struct Converter<const extensions::Extension*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const extensions::Extension* val);
};

template <>
struct Converter<electron::api::ExtensionTabDetails> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::api::ExtensionTabDetails* out);
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_
