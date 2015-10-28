// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_BLINK_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_BLINK_CONVERTER_H_

#include "native_mate/converter.h"

namespace blink {
class WebInputEvent;
class WebMouseEvent;
class WebMouseWheelEvent;
class WebKeyboardEvent;
struct WebDeviceEmulationParams;
struct WebFloatPoint;
struct WebPoint;
struct WebSize;
}

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

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_BLINK_CONVERTER_H_
