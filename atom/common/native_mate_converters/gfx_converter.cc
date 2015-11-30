// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/gfx_converter.h"

#include "native_mate/dictionary.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/screen.h"
#include "ui/gfx/geometry/size.h"

namespace mate {

v8::Local<v8::Value> Converter<gfx::Point>::ToV8(v8::Isolate* isolate,
                                                  const gfx::Point& val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("x", val.x());
  dict.Set("y", val.y());
  return dict.GetHandle();
}

bool Converter<gfx::Point>::FromV8(v8::Isolate* isolate,
                                   v8::Local<v8::Value> val,
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

v8::Local<v8::Value> Converter<gfx::Size>::ToV8(v8::Isolate* isolate,
                                                  const gfx::Size& val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("width", val.width());
  dict.Set("height", val.height());
  return dict.GetHandle();
}

bool Converter<gfx::Size>::FromV8(v8::Isolate* isolate,
                                  v8::Local<v8::Value> val,
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

v8::Local<v8::Value> Converter<gfx::Rect>::ToV8(v8::Isolate* isolate,
                                                 const gfx::Rect& val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("x", val.x());
  dict.Set("y", val.y());
  dict.Set("width", val.width());
  dict.Set("height", val.height());
  return dict.GetHandle();
}

bool Converter<gfx::Rect>::FromV8(v8::Isolate* isolate,
                                  v8::Local<v8::Value> val,
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

template<>
struct Converter<gfx::Display::TouchSupport> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Display::TouchSupport& val) {
    switch (val) {
      case gfx::Display::TOUCH_SUPPORT_AVAILABLE:
        return StringToV8(isolate, "available");
      case gfx::Display::TOUCH_SUPPORT_UNAVAILABLE:
        return StringToV8(isolate, "unavailable");
      default:
        return StringToV8(isolate, "unknown");
    }
  }
};

v8::Local<v8::Value> Converter<gfx::Display>::ToV8(v8::Isolate* isolate,
                                                    const gfx::Display& val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("id", val.id());
  dict.Set("bounds", val.bounds());
  dict.Set("workArea", val.work_area());
  dict.Set("size", val.size());
  dict.Set("workAreaSize", val.work_area_size());
  dict.Set("scaleFactor", val.device_scale_factor());
  dict.Set("rotation", val.RotationAsDegree());
  dict.Set("touchSupport", val.touch_support());
  return dict.GetHandle();
}

}  // namespace mate
