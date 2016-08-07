// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "native_mate/converter.h"
#include "third_party/WebKit/public/web/WebCache.h"
#include "third_party/WebKit/public/web/WebContextMenuData.h"

namespace blink {
class WebInputEvent;
class WebMouseEvent;
class WebMouseWheelEvent;
class WebKeyboardEvent;
struct WebDeviceEmulationParams;
struct WebFindOptions;
struct WebFloatPoint;
struct WebPoint;
struct WebSize;
}  // namespace blink

namespace content {
struct NativeWebKeyboardEvent;
}

namespace mate {

int GetWebInputEventType(v8::Isolate* isolate, v8::Local<v8::Value> val);

template<>
struct Converter<blink::WebInputEvent> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebInputEvent* out);
};

template<>
struct Converter<blink::WebKeyboardEvent> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebKeyboardEvent* out);
};

template<>
struct Converter<content::NativeWebKeyboardEvent> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::NativeWebKeyboardEvent* out);
};

template<>
struct Converter<blink::WebMouseEvent> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebMouseEvent* out);
};

template<>
struct Converter<blink::WebMouseWheelEvent> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebMouseWheelEvent* out);
};

template<>
struct Converter<blink::WebFloatPoint> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebFloatPoint* out);
};

template<>
struct Converter<blink::WebPoint> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebPoint* out);
};

template<>
struct Converter<blink::WebSize> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebSize* out);
};

template<>
struct Converter<blink::WebDeviceEmulationParams> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebDeviceEmulationParams* out);
};

template<>
struct Converter<blink::WebFindOptions> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebFindOptions* out);
};

template<>
struct Converter<blink::WebContextMenuData::MediaType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
      const blink::WebContextMenuData::MediaType& in);
};

template<>
struct Converter<blink::WebContextMenuData::InputFieldType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
      const blink::WebContextMenuData::InputFieldType& in);
};

template<>
struct Converter<blink::WebCache::ResourceTypeStat> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::WebCache::ResourceTypeStat& stat);
};

template<>
struct Converter<blink::WebCache::ResourceTypeStats> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::WebCache::ResourceTypeStats& stats);
};

v8::Local<v8::Value> EditFlagsToV8(v8::Isolate* isolate, int editFlags);
v8::Local<v8::Value> MediaFlagsToV8(v8::Isolate* isolate, int mediaFlags);

}  // namespace mate
