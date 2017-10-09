// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_native_image.h"

#import <Cocoa/Cocoa.h>

#include "base/strings/sys_string_conversions.h"

namespace atom {

namespace api {

mate::Handle<NativeImage> NativeImage::CreateFromNamedImage(
  mate::Arguments* args, const std::string& name) {
  @autoreleasepool {
    NSImage* image = [NSImage imageNamed:base::SysUTF8ToNSString(name)];
    if (!image.valid) {
      return CreateEmpty(args->isolate());
    }

    CGImageRef ref = [image CGImageForProposedRect:nil context:nil hints:nil];
    NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithCGImage:ref];
    [rep setSize:[image size]];
    NSData* pngData = [rep representationUsingType:NSPNGFileType properties:[[NSDictionary alloc] init]];

    return CreateFromPNG(args->isolate(), (char *) [pngData bytes], [pngData length]);
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
