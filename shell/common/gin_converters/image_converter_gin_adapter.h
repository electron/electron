// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_GIN_ADAPTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_GIN_ADAPTER_H_

#include "gin/converter.h"
#include "shell/common/native_mate_converters/image_converter.h"

// TODO(deermichel): replace adapter with real implementation after removing
// mate
// -- this adapter forwards all conversions to the existing mate converter --
// (other direction might be preferred, but this is safer for now :D)

namespace gin {

template <>
struct Converter<gfx::ImageSkia> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::ImageSkia* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_GIN_ADAPTER_H_
