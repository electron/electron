// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_H_

#include "gin/converter.h"
#include "third_party/blink/public/common/context_menu_data/context_menu_data.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/messaging/cloneable_message.h"
#include "third_party/blink/public/common/web_cache/web_cache_resource_type_stats.h"
#include "third_party/blink/public/mojom/loader/referrer.mojom-forward.h"

namespace blink {
class WebMouseEvent;
class WebMouseWheelEvent;
class WebKeyboardEvent;
struct DeviceEmulationParams;
}  // namespace blink

namespace gin {

blink::WebInputEvent::Type GetWebInputEventType(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val);

template <>
struct Converter<blink::WebInputEvent::Type> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebInputEvent::Type* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::WebInputEvent::Type& in);
};

template <>
struct Converter<blink::WebInputEvent> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebInputEvent* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::WebInputEvent& in);
};

template <>
struct Converter<blink::WebKeyboardEvent> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebKeyboardEvent* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::WebKeyboardEvent& in);
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
struct Converter<blink::DeviceEmulationParams> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::DeviceEmulationParams* out);
};

template <>
struct Converter<blink::mojom::ContextMenuDataMediaType> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::mojom::ContextMenuDataMediaType& in);
};

template <>
struct Converter<std::optional<blink::mojom::FormControlType>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const std::optional<blink::mojom::FormControlType>& in);
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
struct Converter<blink::mojom::Referrer> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::mojom::Referrer& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::Referrer* out);
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

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_BLINK_CONVERTER_H_
