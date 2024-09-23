
// Copyright (c) 2024 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_OSR_OSR_PAINT_EVENT_H
#define ELECTRON_SHELL_BROWSER_OSR_OSR_PAINT_EVENT_H

#include "base/functional/callback_helpers.h"
#include "content/public/common/widget_type.h"
#include "media/base/video_types.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/viz/privileged/mojom/compositing/frame_sink_video_capture.mojom.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/native_widget_types.h"

#include <cstdint>

namespace electron {

struct OffscreenNativePixmapPlaneInfo {
  // The strides and offsets in bytes to be used when accessing the buffers
  // via a memory mapping. One per plane per entry. Size in bytes of the
  // plane is necessary to map the buffers.
  uint32_t stride;
  uint64_t offset;
  uint64_t size;

  // File descriptor for the underlying memory object (usually dmabuf).
  int fd;

  OffscreenNativePixmapPlaneInfo() = delete;
  ~OffscreenNativePixmapPlaneInfo();
  OffscreenNativePixmapPlaneInfo(const OffscreenNativePixmapPlaneInfo& other);
  OffscreenNativePixmapPlaneInfo(uint32_t stride,
                                 uint64_t offset,
                                 uint64_t size,
                                 int fd);
};

struct OffscreenReleaserHolder {
  OffscreenReleaserHolder() = delete;
  ~OffscreenReleaserHolder();
  OffscreenReleaserHolder(
      gfx::GpuMemoryBufferHandle gmb_handle,
      mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
          releaser);

  // GpuMemoryBufferHandle, keep the scoped handle alive
  gfx::GpuMemoryBufferHandle gmb_handle;

  // Releaser, hold this to prevent FrameSinkVideoCapturer recycle frame
  mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
      releaser;
};

struct OffscreenSharedTextureValue {
  OffscreenSharedTextureValue();
  ~OffscreenSharedTextureValue();
  OffscreenSharedTextureValue(const OffscreenSharedTextureValue& other);

  // It is user's responsibility to compose popup widget textures.
  content::WidgetType widget_type;

  // The pixel format of the shared texture, RGBA or BGRA depends on platform.
  media::VideoPixelFormat pixel_format;

  // The full dimensions of the video frame data.
  gfx::Size coded_size;

  // A subsection of [0, 0, coded_size().width(), coded_size.height()].
  // In OSR case, it is expected to have the full area of the section.
  gfx::Rect visible_rect;

  // The region of the video frame that capturer would like to populate.
  // In OSR case, it is the same with `dirtyRect` that needs to be painted.
  gfx::Rect content_rect;

  // Extra metadata for the video frame.
  // See comments in src\media\base\video_frame_metadata.h for more details.
  std::optional<gfx::Rect> capture_update_rect;
  std::optional<gfx::Size> source_size;
  std::optional<gfx::Rect> region_capture_rect;

  // The capture timestamp, microseconds since capture start
  int64_t timestamp;

  // The frame count
  int64_t frame_count;

  // Releaser holder
  raw_ptr<OffscreenReleaserHolder> releaser_holder;

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  // On Windows it is a HANDLE to the shared D3D11 texture.
  // On macOS it is a IOSurface* to the shared IOSurface.
  uintptr_t shared_texture_handle;
#elif BUILDFLAG(IS_LINUX)
  std::vector<OffscreenNativePixmapPlaneInfo> planes;
  uint64_t modifier;
#endif
};

typedef std::optional<OffscreenSharedTextureValue> OffscreenSharedTexture;

typedef base::RepeatingCallback<
    void(const gfx::Rect&, const SkBitmap&, const OffscreenSharedTexture&)>
    OnPaintCallback;

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_OSR_OSR_PAINT_EVENT_H
