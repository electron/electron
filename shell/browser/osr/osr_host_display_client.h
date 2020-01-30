// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_OSR_OSR_HOST_DISPLAY_CLIENT_H_
#define SHELL_BROWSER_OSR_OSR_HOST_DISPLAY_CLIENT_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/shared_memory_mapping.h"
#include "components/viz/host/host_display_client.h"
#include "services/viz/privileged/mojom/compositing/layered_window_updater.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/native_widget_types.h"

namespace electron {

typedef base::Callback<void(const gfx::Rect&, const SkBitmap&)> OnPaintCallback;

class LayeredWindowUpdater : public viz::mojom::LayeredWindowUpdater {
 public:
  explicit LayeredWindowUpdater(
      mojo::PendingReceiver<viz::mojom::LayeredWindowUpdater> receiver,
      OnPaintCallback callback);
  ~LayeredWindowUpdater() override;

  void SetActive(bool active);

  // viz::mojom::LayeredWindowUpdater implementation.
  void OnAllocatedSharedMemory(const gfx::Size& pixel_size,
                               base::UnsafeSharedMemoryRegion region) override;
  void Draw(const gfx::Rect& damage_rect, DrawCallback draw_callback) override;

 private:
  OnPaintCallback callback_;
  mojo::Receiver<viz::mojom::LayeredWindowUpdater> receiver_;
  std::unique_ptr<SkCanvas> canvas_;
  bool active_ = false;

#if !defined(WIN32)
  base::WritableSharedMemoryMapping shm_mapping_;
#endif

  DISALLOW_COPY_AND_ASSIGN(LayeredWindowUpdater);
};

class OffScreenHostDisplayClient : public viz::HostDisplayClient {
 public:
  explicit OffScreenHostDisplayClient(gfx::AcceleratedWidget widget,
                                      OnPaintCallback callback);
  ~OffScreenHostDisplayClient() override;

  void SetActive(bool active);

 private:
  void IsOffscreen(IsOffscreenCallback callback) override;

#if defined(OS_MACOSX)
  void OnDisplayReceivedCALayerParams(
      const gfx::CALayerParams& ca_layer_params) override;
#endif

  void CreateLayeredWindowUpdater(
      mojo::PendingReceiver<viz::mojom::LayeredWindowUpdater> receiver)
      override;

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  void DidCompleteSwapWithNewSize(const gfx::Size& size) override;
#endif

  std::unique_ptr<LayeredWindowUpdater> layered_window_updater_;
  OnPaintCallback callback_;
  bool active_ = false;

  DISALLOW_COPY_AND_ASSIGN(OffScreenHostDisplayClient);
};

}  // namespace electron

#endif  // SHELL_BROWSER_OSR_OSR_HOST_DISPLAY_CLIENT_H_
