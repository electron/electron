// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_native_image.h"

#import <Cocoa/Cocoa.h>

#include "base/strings/sys_string_conversions.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace atom {

namespace api {

NSData* bufferFromNSImage(NSImage* image) {
  CGImageRef ref = [image CGImageForProposedRect:nil context:nil hints:nil];
  NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithCGImage:ref];
  [rep setSize:[image size]];
  return [rep representationUsingType:NSPNGFileType properties:[[NSDictionary alloc] init]];
}

double safeShift(double in, double def) {
  if (in >= 0 || in <= 1 || in == def) return in;
  return def;
}

mate::Handle<NativeImage> NativeImage::CreateFromNamedImage(
  mate::Arguments* args, const std::string& name) {
  @autoreleasepool {
    std::vector<double> hsl_shift;
    NSImage* image = [NSImage imageNamed:base::SysUTF8ToNSString(name)];
    if (!image.valid) {
      return CreateEmpty(args->isolate());
    }

    NSData* png_data = bufferFromNSImage(image);

    if (args->GetNext(&hsl_shift) && hsl_shift.size() == 3) {
      gfx::Image gfx_image = gfx::Image::CreateFrom1xPNGBytes(
        reinterpret_cast<const unsigned char*>((char *) [png_data bytes]), [png_data length]);
      color_utils::HSL shift = {
        safeShift(hsl_shift[0], -1),
        safeShift(hsl_shift[1], 0.5),
        safeShift(hsl_shift[2], 0.5)
      };
      png_data = bufferFromNSImage(gfx::Image(
        gfx::ImageSkiaOperations::CreateHSLShiftedImage(
          gfx_image.AsImageSkia(), shift)).CopyNSImage());
    }

    return CreateFromPNG(args->isolate(), (char *) [png_data bytes], [png_data length]);
  }
}

void NativeImage::SetTemplateImage(bool setAsTemplate) {
  [image_.AsNSImage() setTemplate:setAsTemplate];
}

bool NativeImage::IsTemplateImage() {
  return [image_.AsNSImage() isTemplate];
}

}  // namespace api

}  // namespace atom
