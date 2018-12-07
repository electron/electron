// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include <utility>

#include "atom/common/native_mate_converters/gfx_converter.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/gfx/skbitmap_operations.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

constexpr static int kMaxFrameRate = 30;

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 content::WebContents* web_contents,
                                 const FrameCaptureCallback& callback,
                                 bool only_dirty)
    : content::WebContentsObserver(web_contents),
      isolate_(isolate),
      callback_(callback),
      only_dirty_(only_dirty),
      weak_ptr_factory_(this) {
  content::RenderViewHost* rvh = web_contents->GetRenderViewHost();
  if (rvh)
    AttachToHost(rvh->GetWidget());
}

FrameSubscriber::~FrameSubscriber() = default;

void FrameSubscriber::AttachToHost(content::RenderWidgetHost* host) {
  host_ = host;

  // The view can be null if the renderer process has crashed.
  // (https://crbug.com/847363)
  if (!host_->GetView())
    return;

  // Create and configure the video capturer.
  video_capturer_ = host_->GetView()->CreateVideoCapturer();
  video_capturer_->SetResolutionConstraints(
      host_->GetView()->GetViewBounds().size(),
      host_->GetView()->GetViewBounds().size(), true);
  video_capturer_->SetAutoThrottlingEnabled(false);
  video_capturer_->SetMinSizeChangePeriod(base::TimeDelta());
  video_capturer_->SetFormat(media::PIXEL_FORMAT_ARGB,
                             gfx::ColorSpace::CreateREC709());
  video_capturer_->SetMinCapturePeriod(base::TimeDelta::FromSeconds(1) /
                                       kMaxFrameRate);
  video_capturer_->Start(this);
}

void FrameSubscriber::DetachFromHost() {
  if (!host_)
    return;
  video_capturer_.reset();
  host_ = nullptr;
}

void FrameSubscriber::RenderViewCreated(content::RenderViewHost* host) {
  if (!host_)
    AttachToHost(host->GetWidget());
}

void FrameSubscriber::RenderViewDeleted(content::RenderViewHost* host) {
  if (host->GetWidget() == host_) {
    DetachFromHost();
  }
}

void FrameSubscriber::RenderViewHostChanged(content::RenderViewHost* old_host,
                                            content::RenderViewHost* new_host) {
  if ((old_host && old_host->GetWidget() == host_) || (!old_host && !host_)) {
    DetachFromHost();
    AttachToHost(new_host->GetWidget());
  }
}

void FrameSubscriber::OnFrameCaptured(
    base::ReadOnlySharedMemoryRegion data,
    ::media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& update_rect,
    const gfx::Rect& content_rect,
    viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) {
  gfx::Size view_size = host_->GetView()->GetViewBounds().size();
  if (view_size != content_rect.size()) {
    video_capturer_->SetResolutionConstraints(view_size, view_size, true);
    video_capturer_->RequestRefreshFrame();
    return;
  }

  if (!data.IsValid()) {
    callbacks->Done();
    return;
  }
  base::ReadOnlySharedMemoryMapping mapping = data.Map();
  if (!mapping.IsValid()) {
    DLOG(ERROR) << "Shared memory mapping failed.";
    return;
  }
  if (mapping.size() <
      media::VideoFrame::AllocationSize(info->pixel_format, info->coded_size)) {
    DLOG(ERROR) << "Shared memory size was less than expected.";
    return;
  }

  // The SkBitmap's pixels will be marked as immutable, but the installPixels()
  // API requires a non-const pointer. So, cast away the const.
  void* const pixels = const_cast<void*>(mapping.memory());

  // Call installPixels() with a |releaseProc| that: 1) notifies the capturer
  // that this consumer has finished with the frame, and 2) releases the shared
  // memory mapping.
  struct FramePinner {
    // Keeps the shared memory that backs |frame_| mapped.
    base::ReadOnlySharedMemoryMapping mapping;
    // Prevents FrameSinkVideoCapturer from recycling the shared memory that
    // backs |frame_|.
    viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr releaser;
  };

  SkBitmap bitmap;
  bitmap.installPixels(
      SkImageInfo::MakeN32(content_rect.width(), content_rect.height(),
                           kPremul_SkAlphaType),
      pixels,
      media::VideoFrame::RowBytes(media::VideoFrame::kARGBPlane,
                                  info->pixel_format, info->coded_size.width()),
      [](void* addr, void* context) {
        delete static_cast<FramePinner*>(context);
      },
      new FramePinner{std::move(mapping), std::move(callbacks)});
  bitmap.setImmutable();

  Done(content_rect, bitmap);
}

void FrameSubscriber::OnStopped() {}

void FrameSubscriber::Done(const gfx::Rect& damage, const SkBitmap& frame) {
  if (frame.drawsNothing())
    return;

  v8::Locker locker(isolate_);
  v8::HandleScope handle_scope(isolate_);

  const_cast<SkBitmap&>(frame).setAlphaType(kPremul_SkAlphaType);
  const SkBitmap& bitmap = only_dirty_ ? SkBitmapOperations::CreateTiledBitmap(
                                             frame, damage.x(), damage.y(),
                                             damage.width(), damage.height())
                                       : frame;

  size_t rgb_row_size = bitmap.width() * bitmap.bytesPerPixel();
  auto* source = static_cast<const char*>(bitmap.getPixels());

  v8::MaybeLocal<v8::Object> buffer =
      node::Buffer::Copy(isolate_, source, rgb_row_size * bitmap.height());
  auto local_buffer = buffer.ToLocalChecked();

  v8::Local<v8::Value> damage_rect =
      mate::Converter<gfx::Rect>::ToV8(isolate_, damage);

  callback_.Run(local_buffer, damage_rect);
}

}  // namespace api

}  // namespace atom
