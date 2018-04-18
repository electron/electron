// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include "atom/common/native_mate_converters/gfx_converter.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "content/browser/compositor/surface_utils.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 content::RenderWidgetHostView* view,
                                 const FrameCaptureCallback& callback)
    : isolate_(isolate),
      view_(view),
      callback_(callback),
      video_consumer_binding_(this),
      weak_factory_(this) {
  video_capturer_ = CreateVideoCapturer();
  video_capturer_->SetResolutionConstraints(
      view_->GetViewBounds().size(),
      view_->GetViewBounds().size(), true);
  video_capturer_->SetFormat(media::PIXEL_FORMAT_ARGB,
                             media::COLOR_SPACE_UNSPECIFIED);

  viz::mojom::FrameSinkVideoConsumerPtr consumer;
  video_consumer_binding_.Bind(mojo::MakeRequest(&consumer));
  video_capturer_->Start(std::move(consumer));
}

FrameSubscriber::~FrameSubscriber() = default;

void FrameSubscriber::OnFrameCaptured(
    mojo::ScopedSharedBufferHandle buffer,
    uint32_t buffer_size,
    ::media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& update_rect,
    const gfx::Rect& content_rect,
    viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) {
  v8::Locker locker(isolate_);
  v8::HandleScope handle_scope(isolate_);

  gfx::Size view_size = view_->GetViewBounds().size();
  if (view_size != content_rect.size()) {
    video_capturer_->SetResolutionConstraints(view_size, view_size, true);
    video_capturer_->RequestRefreshFrame();
    return;
  }

  if (!buffer.is_valid()) {
    callbacks->Done();
    return;
  }

  mojo::ScopedSharedBufferMapping mapping = buffer->Map(buffer_size);
  if (!mapping) {
    return;
  }

  SkImageInfo image_info = SkImageInfo::MakeN32(
      content_rect.width(), content_rect.height(), kPremul_SkAlphaType);
  SkPixmap pixmap(image_info, mapping.get(),
                  media::VideoFrame::RowBytes(media::VideoFrame::kARGBPlane,
                                              info->pixel_format,
                                              info->coded_size.width()));
  frame_.installPixels(pixmap);
  size_t rgb_row_size = frame_.width() * frame_.bytesPerPixel();
  v8::MaybeLocal<v8::Object> node_buffer =
      node::Buffer::New(isolate_, rgb_row_size * frame_.height());
  auto local_buffer = node_buffer.ToLocalChecked();

  {
    auto* source = static_cast<const unsigned char*>(frame_.getPixels());
    auto* target = node::Buffer::Data(local_buffer);

    for (int y = 0; y < frame_.height(); ++y) {
      memcpy(target, source, rgb_row_size);
      source += frame_.rowBytes();
      target += rgb_row_size;
    }
  }

  v8::Local<v8::Value> damage =
      mate::Converter<gfx::Rect>::ToV8(isolate_, update_rect);

  callback_.Run(local_buffer, damage);

  shared_memory_mapping_ = std::move(mapping);
  shared_memory_releaser_ = std::move(callbacks);
}

void FrameSubscriber::OnTargetLost(const viz::FrameSinkId& frame_sink_id) {}

void FrameSubscriber::OnStopped() {}

viz::mojom::FrameSinkVideoCapturerPtr FrameSubscriber::CreateVideoCapturer() {
  auto* view_base = static_cast<content::RenderWidgetHostViewBase*>(view_);
  viz::mojom::FrameSinkVideoCapturerPtr video_capturer;
  content::GetHostFrameSinkManager()->CreateVideoCapturer(
      mojo::MakeRequest(&video_capturer));
  video_capturer->ChangeTarget(view_base->GetFrameSinkId());
  return video_capturer;
}

}  // namespace api

}  // namespace atom
