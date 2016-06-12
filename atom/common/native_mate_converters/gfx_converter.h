// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "native_mate/converter.h"

namespace gfx {
class Point;
class Size;
class Rect;
class Display;
}

namespace mate {

template<>
struct Converter<gfx::Point> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Point& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Point* out);
};

template<>
struct Converter<gfx::Size> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Size& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Size* out);
};

template<>
struct Converter<gfx::Rect> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Rect& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Rect* out);
};

template<>
struct Converter<gfx::Display> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Display& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Display* out);
};

}  // namespace mate
