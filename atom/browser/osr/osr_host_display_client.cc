// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_host_display_client.h"

#include "base/memory/shared_memory.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_sizes.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "skia/ext/platform_canvas.h"
#include "skia/ext/skia_utils_win.h"

#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/src/core/SkDevice.h"
#include "ui/gfx/skia_util.h"

namespace atom {

LayeredWindowUpdater::LayeredWindowUpdater(
    viz::mojom::LayeredWindowUpdaterRequest request,
    OnPaintCallback callback)
    : callback_(callback), binding_(this, std::move(request)) {}

LayeredWindowUpdater::~LayeredWindowUpdater() = default;

void LayeredWindowUpdater::OnAllocatedSharedMemory(
    const gfx::Size& pixel_size,
    mojo::ScopedSharedBufferHandle scoped_buffer_handle) {
  canvas_.reset();

  // Make sure |pixel_size| is sane.
  size_t expected_bytes;
  bool size_result = viz::ResourceSizes::MaybeSizeInBytes(
      pixel_size, viz::ResourceFormat::RGBA_8888, &expected_bytes);
  if (!size_result)
    return;

  base::SharedMemoryHandle shm_handle;
  MojoResult unwrap_result = mojo::UnwrapSharedMemoryHandle(
      std::move(scoped_buffer_handle), &shm_handle, nullptr, nullptr);
  if (unwrap_result != MOJO_RESULT_OK)
    return;

  base::SharedMemory shm(shm_handle, false);
#if defined(WIN32)
  canvas_ = skia::CreatePlatformCanvasWithSharedSection(
      pixel_size.width(), pixel_size.height(), false, shm.handle().GetHandle(),
      skia::CRASH_ON_FAILURE);
#else
  canvas_ = skia::CreatePlatformCanvasWithPixels(
      pixel_size.width(), pixel_size.height(), false, shm.memory(),
      skia::CRASH_ON_FAILURE);
#endif

  shm_handle.Close();
}

void LayeredWindowUpdater::Draw(const gfx::Rect& damage_rect,
                                DrawCallback draw_callback) {
  SkPixmap pixmap;
  SkBitmap bitmap;

  if (canvas_->peekPixels(&pixmap)) {
    bitmap.installPixels(pixmap);
    callback_.Run(damage_rect, bitmap);
  }

  std::move(draw_callback).Run();
}

OffScreenHostDisplayClient::OffScreenHostDisplayClient(
    gfx::AcceleratedWidget widget,
    OnPaintCallback callback)
    : viz::HostDisplayClient(widget), callback_(callback) {}
OffScreenHostDisplayClient::~OffScreenHostDisplayClient() {}

void OffScreenHostDisplayClient::IsOffscreen(IsOffscreenCallback callback) {
  std::move(callback).Run(true);
}

#if defined(OS_WIN)
void OffScreenHostDisplayClient::CreateLayeredWindowUpdater(
    viz::mojom::LayeredWindowUpdaterRequest request) {
  layered_window_updater_ =
      std::make_unique<LayeredWindowUpdater>(std::move(request), callback_);
}
#endif

}  // namespace atom
