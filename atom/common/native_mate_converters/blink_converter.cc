// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/blink_converter.h"

#include "base/strings/string_util.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebDeviceEmulationParams.h"

namespace mate {

bool Converter<blink::WebFloatPoint>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val, blink::WebFloatPoint* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("x", &out->x) && dict.Get("y", &out->y);
}

bool Converter<blink::WebPoint>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val, blink::WebPoint* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("x", &out->x) && dict.Get("y", &out->y);
}

bool Converter<blink::WebSize>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val, blink::WebSize* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("width", &out->width) && dict.Get("height", &out->height);
}

bool Converter<blink::WebDeviceEmulationParams>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val,
    blink::WebDeviceEmulationParams* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  std::string screen_position;
  if (dict.Get("screenPosition", &screen_position)) {
    screen_position = base::StringToLowerASCII(screen_position);
    if (screen_position == "mobile")
      out->screenPosition = blink::WebDeviceEmulationParams::Mobile;
    else if (screen_position == "desktop")
      out->screenPosition = blink::WebDeviceEmulationParams::Desktop;
    else
      return false;
  }

  dict.Get("screenSize", &out->screenSize);
  dict.Get("viewPosition", &out->viewPosition);
  dict.Get("deviceScaleFactor", &out->deviceScaleFactor);
  dict.Get("viewSize", &out->viewSize);
  dict.Get("fitToView", &out->fitToView);
  dict.Get("offset", &out->offset);
  dict.Get("scale", &out->scale);
  return true;
}

}  // namespace mate
