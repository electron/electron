// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_GFX_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_GFX_CONVERTER_H_

#include "native_mate/converter.h"
#include "shell/common/gin_converters/gfx_converter.h"

namespace mate {

template <>
struct Converter<gfx::Point> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gfx::Point& val) {
    return gin::ConvertToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Point* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

template <>
struct Converter<gfx::PointF> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gfx::PointF& val) {
    return gin::ConvertToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::PointF* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

template <>
struct Converter<gfx::Size> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Size& val) {
    return gin::ConvertToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Size* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

template <>
struct Converter<gfx::Rect> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Rect& val) {
    return gin::ConvertToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Rect* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

template <>
struct Converter<display::Display> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const display::Display& val) {
    return gin::ConvertToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     display::Display* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_GFX_CONVERTER_H_
