// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_GFX_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_GFX_CONVERTER_H_

#include "native_mate/dictionary.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/screen.h"
#include "ui/gfx/size.h"

namespace mate {

template<>
struct Converter<gfx::Point> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Point& val) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("x", val.x());
    dict.Set("y", val.y());
    return dict.GetHandle();
  }
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     gfx::Point* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    int x, y;
    if (!dict.Get("x", &x) || !dict.Get("y", &y))
      return false;
    *out = gfx::Point(x, y);
    return true;
  }
};

template<>
struct Converter<gfx::Size> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Size& val) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("width", val.width());
    dict.Set("height", val.height());
    return dict.GetHandle();
  }
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     gfx::Size* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    int width, height;
    if (!dict.Get("width", &width) || !dict.Get("height", &height))
      return false;
    *out = gfx::Size(width, height);
    return true;
  }
};

template<>
struct Converter<gfx::Rect> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Rect& val) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("x", val.x());
    dict.Set("y", val.y());
    dict.Set("width", val.width());
    dict.Set("height", val.height());
    return dict.GetHandle();
  }
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     gfx::Rect* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    int x, y, width, height;
    if (!dict.Get("x", &x) || !dict.Get("y", &y) ||
        !dict.Get("width", &width) || !dict.Get("height", &height))
      return false;
    *out = gfx::Rect(x, y, width, height);
    return true;
  }
};

template<>
struct Converter<gfx::Display> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Display& display) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("bounds", display.bounds());
    dict.Set("workArea", display.work_area());
    dict.Set("size", display.size());
    dict.Set("workAreaSize", display.work_area_size());
    dict.Set("scaleFactor", display.device_scale_factor());
    return dict.GetHandle();
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_GFX_CONVERTER_H_
