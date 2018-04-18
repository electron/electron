// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
#define ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/render_widget_host_view.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_video_capture.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "v8/include/v8.h"

namespace atom {

namespace api {

class FrameSubscriber : public viz::mojom::FrameSinkVideoConsumer {
 public:
  using FrameCaptureCallback =
      base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>;

  FrameSubscriber(v8::Isolate* isolate,
                  content::RenderWidgetHostView* view,
                  const FrameCaptureCallback& callback);
  ~FrameSubscriber() override;

 private:
  void OnFrameCaptured(
    mojo::ScopedSharedBufferHandle buffer,
    uint32_t buffer_size,
    ::media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& update_rect,
    const gfx::Rect& content_rect,
    viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) override;
  void OnTargetLost(const viz::FrameSinkId& frame_sink_id) override;
  void OnStopped() override;
  viz::mojom::FrameSinkVideoCapturerPtr CreateVideoCapturer();

  v8::Isolate* isolate_;
  content::RenderWidgetHostView* view_;
  FrameCaptureCallback callback_;

  SkBitmap frame_;

  // This object keeps the shared memory that backs |frame_| mapped.
  mojo::ScopedSharedBufferMapping shared_memory_mapping_;

  // This object prevents FrameSinkVideoCapturer from recycling the shared
  // memory that backs |frame_|.
  viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr shared_memory_releaser_;

  viz::mojom::FrameSinkVideoCapturerPtr video_capturer_;
  mojo::Binding<viz::mojom::FrameSinkVideoConsumer> video_consumer_binding_;

  base::WeakPtrFactory<FrameSubscriber> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FrameSubscriber);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
