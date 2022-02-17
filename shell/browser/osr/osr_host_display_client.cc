// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/osr/osr_host_display_client.h"

#include <utility>

#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_sizes.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/src/core/SkDevice.h"
#include "ui/gfx/skia_util.h"

#if BUILDFLAG(IS_WIN)
#include "skia/ext/skia_utils_win.h"
#endif

namespace electron {

LayeredWindowUpdater::LayeredWindowUpdater(
    mojo::PendingReceiver<viz::mojom::LayeredWindowUpdater> receiver,
    OnPaintCallback callback)
    : callback_(callback), receiver_(this, std::move(receiver)) {}

LayeredWindowUpdater::~LayeredWindowUpdater() = default;

void LayeredWindowUpdater::SetActive(bool active) {
  active_ = active;
}

void LayeredWindowUpdater::OnAllocatedSharedMemory(
    const gfx::Size& pixel_size,
    base::UnsafeSharedMemoryRegion region) {
  canvas_.reset();

  if (!region.IsValid())
    return;

  // Make sure |pixel_size| is sane.
  size_t expected_bytes;
  bool size_result = viz::ResourceSizes::MaybeSizeInBytes(
      pixel_size, viz::ResourceFormat::RGBA_8888, &expected_bytes);
  if (!size_result)
    return;

#if defined(WIN32)
  canvas_ = skia::CreatePlatformCanvasWithSharedSection(
      pixel_size.width(), pixel_size.height(), false,
      region.GetPlatformHandle(), skia::CRASH_ON_FAILURE);
#else
  shm_mapping_ = region.Map();
  if (!shm_mapping_.IsValid()) {
    DLOG(ERROR) << "Failed to map shared memory region";
    return;
  }

  canvas_ = skia::CreatePlatformCanvasWithPixels(
      pixel_size.width(), pixel_size.height(), false,
      static_cast<uint8_t*>(shm_mapping_.memory()), skia::CRASH_ON_FAILURE);
#endif
}

void LayeredWindowUpdater::Draw(const gfx::Rect& damage_rect,
                                DrawCallback draw_callback) {
  SkPixmap pixmap;
  SkBitmap bitmap;

  if (active_ && canvas_->peekPixels(&pixmap)) {
    bitmap.installPixels(pixmap);
    callback_.Run(damage_rect, bitmap);
  }

  std::move(draw_callback).Run();
}

OffScreenHostDisplayClient::OffScreenHostDisplayClient(
    gfx::AcceleratedWidget widget,
    OnPaintCallback callback)
    : viz::HostDisplayClient(widget), callback_(callback) {}
OffScreenHostDisplayClient::~OffScreenHostDisplayClient() = default;

void OffScreenHostDisplayClient::SetActive(bool active) {
  active_ = active;
  if (layered_window_updater_) {
    layered_window_updater_->SetActive(active_);
  }
}

void OffScreenHostDisplayClient::CreateLayeredWindowUpdater(
    mojo::PendingReceiver<viz::mojom::LayeredWindowUpdater> receiver) {
  layered_window_updater_ =
      std::make_unique<LayeredWindowUpdater>(std::move(receiver), callback_);
  layered_window_updater_->SetActive(active_);
}

#if BUILDFLAG(IS_LINUX) && !BUILDFLAG(IS_CHROMEOS)
void OffScreenHostDisplayClient::DidCompleteSwapWithNewSize(
    const gfx::Size& size) {}
#endif

}  // namespace electron
