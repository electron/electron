// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_

#include "gin/converter.h"
#include "shell/browser/api/electron_api_base_window.h"

namespace gin {

template <>
struct Converter<electron::NativeWindow*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::NativeWindow** out) {
    // null would be transferred to nullptr.
    if (val->IsNull()) {
      *out = nullptr;
      return true;
    }

    electron::api::BaseWindow* window;
    if (!gin::Converter<electron::api::BaseWindow*>::FromV8(isolate, val,
                                                            &window))
      return false;
    *out = window->window();
    return true;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_
