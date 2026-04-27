// Copyright (c) 2021 Slack Technologies, LLC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/media_converter.h"

#include <string_view>

#include "base/notreached.h"
#include "content/public/browser/media_stream_request.h"
#include "content/public/browser/render_frame_host.h"
#include "gin/data_object_builder.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom.h"

namespace {

std::string_view PreferredDisplaySurfaceToString(
    blink::mojom::PreferredDisplaySurface surface) {
  switch (surface) {
    case blink::mojom::PreferredDisplaySurface::NO_PREFERENCE:
      return "none";
    case blink::mojom::PreferredDisplaySurface::MONITOR:
      return "monitor";
    case blink::mojom::PreferredDisplaySurface::WINDOW:
      return "window";
    case blink::mojom::PreferredDisplaySurface::BROWSER:
      return "browser";
  }
  NOTREACHED();
}

}  // namespace

namespace gin {

v8::Local<v8::Value> Converter<content::MediaStreamRequest>::ToV8(
    v8::Isolate* isolate,
    const content::MediaStreamRequest& request) {
  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      request.render_process_id, request.render_frame_id);
  return gin::DataObjectBuilder(isolate)
      .Set("frame", rfh)
      .Set("securityOrigin", request.security_origin)
      .Set("userGesture", request.user_gesture)
      .Set("videoRequested",
           request.video_type != blink::mojom::MediaStreamType::NO_SERVICE)
      .Set("audioRequested",
           request.audio_type != blink::mojom::MediaStreamType::NO_SERVICE)
      .Set("preferredDisplaySurface",
           PreferredDisplaySurfaceToString(
               request.preferred_display_surface))
      .Build();
}

}  // namespace gin
