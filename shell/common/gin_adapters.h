// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_ADAPTERS_H_
#define ELECTRON_SHELL_COMMON_GIN_ADAPTERS_H_

#include <utility>

#include "gin/converter.h"

namespace gin {

// Make it possible to convert move-only types.
template <typename T>
v8::Local<v8::Value> ConvertToV8(v8::Isolate* isolate, T&& input) {
  return Converter<typename std::remove_reference<T>::type>::ToV8(
      isolate, std::forward<T>(input));
}

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_ADAPTERS_H_
