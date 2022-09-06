// Copyright (c) 2021 Slack Technologies, LLC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_MEDIA_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_MEDIA_CONVERTER_H_

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

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_MEDIA_CONVERTER_H_
