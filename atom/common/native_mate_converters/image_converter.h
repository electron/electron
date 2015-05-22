// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_IMAGE_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_IMAGE_CONVERTER_H_

#include "native_mate/converter.h"

namespace gfx {
class Image;
class ImageSkia;
}

namespace mate {

template<>
struct Converter<gfx::ImageSkia> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::ImageSkia* out);
};

template<>
struct Converter<gfx::Image> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Image* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Image& val);
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_IMAGE_CONVERTER_H_
