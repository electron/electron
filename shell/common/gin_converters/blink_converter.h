// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_H_

#include "gin/converter.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/messaging/cloneable_message.h"
#include "third_party/blink/public/common/web_cache/web_cache_resource_type_stats.h"
#include "third_party/blink/public/web/web_context_menu_data.h"

namespace blink {
class WebMouseEvent;
class WebMouseWheelEvent;
class WebKeyboardEvent;
struct WebDeviceEmulationParams;
struct WebPoint;
struct WebSize;
}  // namespace blink

namespace gin {

blink::WebInputEvent::Type GetWebInputEventType(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val);

template <>
struct Converter<blink::WebInputEvent> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebInputEvent* out);
};

template <>
struct Converter<blink::WebKeyboardEvent> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebKeyboardEvent* out);
};

template <>
struct Converter<blink::WebMouseEvent> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebMouseEvent* out);
};

template <>
struct Converter<blink::WebMouseWheelEvent> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebMouseWheelEvent* out);
};

template <>
struct Converter<blink::WebPoint> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebPoint* out);
};

template <>
struct Converter<blink::WebSize> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebSize* out);
};

template <>
struct Converter<blink::WebDeviceEmulationParams> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebDeviceEmulationParams* out);
};

template <>
struct Converter<blink::ContextMenuDataMediaType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::ContextMenuDataMediaType& in);
};

template <>
struct Converter<blink::ContextMenuDataInputFieldType> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::ContextMenuDataInputFieldType& in);
};

template <>
struct Converter<blink::WebCacheResourceTypeStat> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::WebCacheResourceTypeStat& stat);
};

template <>
struct Converter<blink::WebCacheResourceTypeStats> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::WebCacheResourceTypeStats& stats);
};

template <>
struct Converter<network::mojom::ReferrerPolicy> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const network::mojom::ReferrerPolicy& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     network::mojom::ReferrerPolicy* out);
};

template <>
struct Converter<blink::CloneableMessage> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::CloneableMessage& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::CloneableMessage* out);
};

v8::Local<v8::Value> EditFlagsToV8(v8::Isolate* isolate, int editFlags);
v8::Local<v8::Value> MediaFlagsToV8(v8::Isolate* isolate, int mediaFlags);

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_H_
