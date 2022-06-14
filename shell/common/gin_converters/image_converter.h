// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_H_

#include "gin/converter.h"

namespace gfx {
class Image;
class ImageSkia;
}  // namespace gfx

namespace gin {

template <>
struct Converter<gfx::ImageSkia> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::ImageSkia* out);
};

template <>
struct Converter<gfx::Image> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Image* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Image& val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_H_
