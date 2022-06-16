// Copyright (c) 2021 Slack Technologies, LLC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_MEDIA_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_MEDIA_CONVERTER_H_

#include "gin/converter.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom-forward.h"

namespace content {
struct MediaStreamRequest;
}

namespace gin {

template <>
struct Converter<content::MediaStreamRequest> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::MediaStreamRequest& request);
};

template <>
struct Converter<blink::MediaStreamDevice> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::MediaStreamDevice* out);
};

template <>
struct Converter<blink::mojom::StreamDevicesPtr> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::StreamDevicesPtr* out);
};

template <>
struct Converter<blink::mojom::MediaStreamRequestResult> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::MediaStreamRequestResult* out);
};

template <>
struct Converter<blink::MediaStreamRequestType> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::MediaStreamRequestType& request_type);
};

template <>
struct Converter<blink::mojom::MediaStreamType> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::mojom::MediaStreamType& stream_type);

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::MediaStreamType* out);
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_MEDIA_CONVERTER_H_
