// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_

#include "gin/converter.h"

namespace extensions {
struct ElectronExtensionInfo;
}

namespace gin {

template <>
struct Converter<extensions::ElectronExtensionInfo> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const extensions::ElectronExtensionInfo& val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_
