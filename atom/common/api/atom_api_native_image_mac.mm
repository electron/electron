// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_native_image.h"

#import <Cocoa/Cocoa.h>

namespace atom {

namespace api {

// static
void NativeImage::MakeTemplateImage(gfx::Image* image) {
  [image->AsNSImage() setTemplate:YES];
}

}  // namespace api

}  // namespace atom
