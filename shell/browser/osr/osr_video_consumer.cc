// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/osr/osr_video_consumer.h"

#include <utility>

#include "media/base/limits.h"
#include "media/base/video_frame_metadata.h"
#include "media/capture/mojom/video_capture_buffer.mojom.h"
#include "media/capture/mojom/video_capture_types.mojom.h"
#include "services/viz/privileged/mojom/compositing/frame_sink_video_capture.mojom-shared.h"
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/skbitmap_operations.h"

namespace electron {

OffScreenVideoConsumer::OffScreenVideoConsumer(
    OffScreenRenderWidgetHostView* view,
    OnPaintCallback callback)
    : callback_(callback),
      view_(view),
      video_capturer_(view->CreateVideoCapturer()) {
  video_capturer_->SetAutoThrottlingEnabled(false);
  video_capturer_->SetMinSizeChangePeriod(base::TimeDelta());
  video_capturer_->SetFormat(media::PIXEL_FORMAT_ARGB);

  // Disable any constraint to make the capture_size keep same with source_size,
  // which will not have a letterbox no matter how size changed. It can also
  // prevent the delay that caused by comparing content_rect with view's size
  // when received a new frame and reset the resolution change then request a
  // new frame.
  video_capturer_->SetResolutionConstraints(
      gfx::Size(1, 1),
      gfx::Size(media::limits::kMaxDimension, media::limits::kMaxDimension),
      false);

  SetFrameRate(view_->frame_rate());
}

OffScreenVideoConsumer::~OffScreenVideoConsumer() = default;

void OffScreenVideoConsumer::SetActive(bool active) {
  if (active) {
    video_capturer_->Start(
        this, view_->offscreen_use_shared_texture()
                  ? viz::mojom::BufferFormatPreference::kPreferGpuMemoryBuffer
                  : viz::mojom::BufferFormatPreference::kDefault);
  } else {
    video_capturer_->Stop();
  }
}

void OffScreenVideoConsumer::SetFrameRate(int frame_rate) {
  video_capturer_->SetMinCapturePeriod(base::Seconds(1) / frame_rate);
}

void OffScreenVideoConsumer::OnFrameCaptured(
    ::media::mojom::VideoBufferHandlePtr data,
    ::media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& content_rect,
    mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
        callbacks) {
  // Since we don't call ProvideFeedback, just need Done to release the frame,
  // there's no need to call the callbacks, see in_flight_frame_delivery.cc
  // The destructor will call Done for us once the pipe closed.

  // Offscreen using GPU shared texture
  if (view_->offscreen_use_shared_texture()) {
    CHECK(data->is_gpu_memory_buffer_handle());

    auto& orig_handle = data->get_gpu_memory_buffer_handle();
    CHECK(!orig_handle.is_null());

    // Clone the handle to support keep the handle alive after the callback
    auto gmb_handle = orig_handle.Clone();

    OffscreenSharedTextureValue texture;
    texture.pixel_format = info->pixel_format;
    texture.coded_size = info->coded_size;
    texture.visible_rect = info->visible_rect;
    texture.timestamp = info->timestamp.InMicroseconds();
    texture.frame_count = info->metadata.capture_counter.value_or(0);
    texture.widget_type = view_->GetWidgetType();

#if BUILDFLAG(IS_WIN)
    texture.shared_texture_handle =
        reinterpret_cast<uintptr_t>(gmb_handle.dxgi_handle.Get());
#elif BUILDFLAG(IS_APPLE)
    texture.shared_texture_handle =
        reinterpret_cast<uintptr_t>(gmb_handle.io_surface.get());
#elif BUILDFLAG(IS_LINUX)
    auto& native_pixmap = gmb_handle.native_pixmap_handle;
    texture.modifier = native_pixmap.modifier;
    for (const auto& plane : native_pixmap.planes) {
      texture.planes.emplace_back(plane.stride, plane.offset, plane.size,
                                  plane.fd.get());
    }
#endif

    // The release holder will be released from JS side when `release` called
    texture.releaser_holder = new OffscreenReleaserHolder(std::move(gmb_handle),
                                                          std::move(callbacks));

    callback_.Run(content_rect, {}, texture);
    return;
  }

  // Regular shared texture capture using shared memory
  auto& data_region = data->get_read_only_shmem_region();

  if (!data_region.IsValid()) {
    return;
  }

  base::ReadOnlySharedMemoryMapping mapping = data_region.Map();
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
    mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
        releaser;
  };

  SkBitmap bitmap;
  bitmap.installPixels(
      SkImageInfo::MakeN32(content_rect.width(), content_rect.height(),
                           kPremul_SkAlphaType),
      pixels,
      media::VideoFrame::RowBytes(media::VideoFrame::Plane::kARGB,
                                  info->pixel_format, info->coded_size.width()),
      [](void* addr, void* context) {
        delete static_cast<FramePinner*>(context);
      },
      new FramePinner{std::move(mapping), std::move(callbacks)});
  bitmap.setImmutable();

  // Since update_rect is already offset-ed with same origin of content_rect,
  // there's nothing more to do with the imported bitmap.
  std::optional<gfx::Rect> update_rect = info->metadata.capture_update_rect;
  if (!update_rect.has_value() || update_rect->IsEmpty()) {
    update_rect = content_rect;
  }

  callback_.Run(*update_rect, bitmap, {});
}

}  // namespace electron
