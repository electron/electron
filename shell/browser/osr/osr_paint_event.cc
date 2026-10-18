
// Copyright (c) 2024 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "shell/browser/osr/osr_paint_event.h"

namespace electron {

OffscreenNativePixmapPlaneInfo::~OffscreenNativePixmapPlaneInfo() = default;
OffscreenNativePixmapPlaneInfo::OffscreenNativePixmapPlaneInfo(
    const OffscreenNativePixmapPlaneInfo& other) = default;
OffscreenNativePixmapPlaneInfo::OffscreenNativePixmapPlaneInfo(uint32_t stride,
                                                               uint64_t offset,
                                                               uint64_t size,
                                                               int fd)
    : stride(stride), offset(offset), size(size), fd(fd) {}

OffscreenReleaserHolder::~OffscreenReleaserHolder() = default;
OffscreenReleaserHolder::OffscreenReleaserHolder(
    gfx::GpuMemoryBufferHandle gmb_handle,
    mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
        releaser)
    : gmb_handle(std::move(gmb_handle)), releaser(std::move(releaser)) {}

OffscreenSharedTextureValue::OffscreenSharedTextureValue() = default;
OffscreenSharedTextureValue::~OffscreenSharedTextureValue() = default;
OffscreenSharedTextureValue::OffscreenSharedTextureValue(
    const OffscreenSharedTextureValue& other) = default;

}  // namespace electron
