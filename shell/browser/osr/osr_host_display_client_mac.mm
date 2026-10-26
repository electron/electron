// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "shell/browser/osr/osr_host_display_client.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImageInfo.h"

#include <IOSurface/IOSurface.h>

namespace electron {

void OffScreenHostDisplayClient::OnDisplayReceivedCALayerParams(
    const gfx::CALayerParams ca_layer_params) {
  if (ca_layer_params.IsEmpty())
    return;

  base::apple::ScopedCFTypeRef<IOSurfaceRef> io_surface(
      IOSurfaceLookupFromMachPort(ca_layer_params.io_surface_mach_port.get()));
  if (!io_surface)
    return;

  // |pixel_size| arrives over Mojo independently of the IOSurface. Only wrap
  // the surface if it actually backs every row the bitmap reads.
  const gfx::Size& pixel_size = ca_layer_params.pixel_size;
  const size_t stride = IOSurfaceGetBytesPerRow(io_surface.get());
  const SkImageInfo image_info = SkImageInfo::MakeN32(
      pixel_size.width(), pixel_size.height(), kPremul_SkAlphaType);
  const size_t required_bytes = image_info.computeByteSize(stride);
  if (pixel_size.IsEmpty() ||
      IOSurfaceGetBytesPerElement(io_surface.get()) != 4 ||
      IOSurfaceGetWidth(io_surface.get()) <
          static_cast<size_t>(pixel_size.width()) ||
      IOSurfaceGetHeight(io_surface.get()) <
          static_cast<size_t>(pixel_size.height()) ||
      SkImageInfo::ByteSizeOverflowed(required_bytes) ||
      required_bytes > IOSurfaceGetAllocSize(io_surface.get())) {
    DLOG(ERROR) << "pixel_size " << pixel_size.ToString()
                << " is not backed by the IOSurface";
    return;
  }

  SkBitmap bitmap;
  bitmap.installPixels(image_info, IOSurfaceGetBaseAddress(io_surface.get()),
                       stride);
  bitmap.setImmutable();
  callback_.Run(ca_layer_params.damage, bitmap, {});
}

}  // namespace electron
