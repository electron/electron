// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_video_consumer.h"

#include <utility>

#include "atom/browser/osr/osr_render_widget_host_view.h"
#include "media/base/video_frame_metadata.h"
#include "media/capture/mojom/video_capture_types.mojom.h"
#include "ui/gfx/skbitmap_operations.h"

namespace atom {

OffScreenVideoConsumer::OffScreenVideoConsumer(
    OffScreenRenderWidgetHostView* view,
    OnPaintCallback callback)
    : callback_(callback),
      view_(view),
      video_capturer_(view->CreateVideoCapturer()),
      weak_ptr_factory_(this) {
  video_capturer_->SetResolutionConstraints(view_->SizeInPixels(),
                                            view_->SizeInPixels(), true);
  video_capturer_->SetAutoThrottlingEnabled(false);
  video_capturer_->SetMinSizeChangePeriod(base::TimeDelta());
  video_capturer_->SetFormat(media::PIXEL_FORMAT_ARGB,
                             gfx::ColorSpace::CreateREC709());
  SetFrameRate(view_->GetFrameRate());
}

OffScreenVideoConsumer::~OffScreenVideoConsumer() = default;

void OffScreenVideoConsumer::SetActive(bool active) {
  if (active) {
    video_capturer_->Start(this);
  } else {
    video_capturer_->Stop();
  }
}

void OffScreenVideoConsumer::SetFrameRate(int frame_rate) {
  video_capturer_->SetMinCapturePeriod(base::TimeDelta::FromSeconds(1) /
                                       frame_rate);
}

void OffScreenVideoConsumer::SizeChanged() {
  video_capturer_->SetResolutionConstraints(view_->SizeInPixels(),
                                            view_->SizeInPixels(), true);
  video_capturer_->RequestRefreshFrame();
}

void OffScreenVideoConsumer::OnFrameCaptured(
    base::ReadOnlySharedMemoryRegion data,
    ::media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& update_rect,
    const gfx::Rect& content_rect,
    viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) {
  if (!CheckContentRect(content_rect)) {
    gfx::Size view_size = view_->SizeInPixels();
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

  media::VideoFrameMetadata metadata;
  metadata.MergeInternalValuesFrom(info->metadata);

  callback_.Run(update_rect, bitmap);
}

void OffScreenVideoConsumer::OnStopped() {}

bool OffScreenVideoConsumer::CheckContentRect(const gfx::Rect& content_rect) {
  gfx::Size view_size = view_->SizeInPixels();
  gfx::Size content_size = content_rect.size();

  if (std::abs(view_size.width() - content_size.width()) > 2) {
    return false;
  }

  if (std::abs(view_size.height() - content_size.height()) > 2) {
    return false;
  }

  return true;
}

}  // namespace atom
