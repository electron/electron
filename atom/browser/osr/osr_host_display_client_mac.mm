// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_host_display_client.h"

#include <IOSurface/IOSurface.h>

namespace atom {

void OffScreenHostDisplayClient::OnDisplayReceivedCALayerParams(
    const gfx::CALayerParams& ca_layer_params) {
  if (!ca_layer_params.is_empty) {
    base::ScopedCFTypeRef<IOSurfaceRef> io_surface(
        IOSurfaceLookupFromMachPort(ca_layer_params.io_surface_mach_port));

    // IOReturn io_result = IOSurfaceLock(
    //     io_surface, kIOSurfaceLockReadOnly | kIOSurfaceLockAvoidSync,
    //     nullptr);
    // if (io_result) {
    //   DLOG(ERROR) << "Failed to lock IOSurface " << io_result;
    //   return;
    // }

    gfx::Size pixel_size_ = ca_layer_params.pixel_size;
    void* pixels = static_cast<void*>(IOSurfaceGetBaseAddress(io_surface));
    size_t stride = IOSurfaceGetBytesPerRow(io_surface);

    struct IOSurfacePinner {
      base::ScopedCFTypeRef<IOSurfaceRef> io_surface;
    };

    SkBitmap bitmap;
    bitmap.installPixels(
        SkImageInfo::MakeN32(pixel_size_.width(), pixel_size_.height(),
                             kPremul_SkAlphaType),
        pixels, stride);
    // [](void*, void* context) {
    //   auto* surface_pinner = static_cast<IOSurfacePinner*>(context);
    //   IOReturn io_result = IOSurfaceUnlock(surface_pinner->io_surface,
    //     kIOSurfaceLockAvoidSync, nullptr);
    //   if (io_result)
    //     DLOG(ERROR) << "Failed to unlock IOSurface " << io_result;
    //   delete surface_pinner;
    // },
    // new IOSurfacePinner{io_surface});
    bitmap.setImmutable();
    callback_.Run(ca_layer_params.damage, bitmap);
  }
}

}  // namespace atom
