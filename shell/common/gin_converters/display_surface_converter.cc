// Copyright (c) 2021 Slack Technologies, LLC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/display_surface_converter.h"

#include "content/public/browser/media_stream_request.h"
#include "gin/data_object_builder.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom.h"

namespace gin {

v8::Local<v8::Value> Converter<blink::mojom::PreferredDisplaySurface>::ToV8(v8::Isolate* isolate,
                                  blink::mojom::PreferredDisplaySurface type) {
  switch (type) {
    case blink::mojom::PreferredDisplaySurface::NO_PREFERENCE:
      return StringToV8(isolate, "no_preference");
    case blink::mojom::PreferredDisplaySurface::MONITOR:
      return StringToV8(isolate, "monitor");
    case blink::mojom::PreferredDisplaySurface::WINDOW:
      return StringToV8(isolate, "window");
    default:
      return StringToV8(isolate, "unknown");
  }
}

}  // namespace gin
