// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GFX_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GFX_CONVERTER_H_

#include "gin/converter.h"

namespace display {
class Display;
}

namespace gfx {
class Point;
class PointF;
class Size;
class Rect;
enum class ResizeEdge;
}  // namespace gfx

namespace gin {

template <>
struct Converter<gfx::Point> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Point& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Point* out);
};

template <>
struct Converter<gfx::PointF> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gfx::PointF& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::PointF* out);
};

template <>
struct Converter<gfx::Size> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Size& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Size* out);
};

template <>
struct Converter<gfx::Rect> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Rect& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Rect* out);
};

template <>
struct Converter<display::Display> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const display::Display& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     display::Display* out);
};

template <>
struct Converter<gfx::ResizeEdge> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gfx::ResizeEdge& val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GFX_CONVERTER_H_
