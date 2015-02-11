// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_native_image.h"

#import <Cocoa/Cocoa.h>

#include "base/files/file_path.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

namespace atom {

namespace api {

namespace {

bool IsTemplateImage(const base::FilePath& path) {
  return (MatchPattern(path.value(), "*Template.*") ||
          MatchPattern(path.value(), "*Template@*x.*"));
}

}  // namespace

// static
mate::Handle<NativeImage> NativeImage::CreateFromPath(
    v8::Isolate* isolate, const base::FilePath& path) {
  base::scoped_nsobject<NSImage> image([[NSImage alloc]
      initByReferencingFile:base::mac::FilePathToNSString(path)]);
  if (IsTemplateImage(path))
    [image setTemplate:YES];
  return Create(isolate, gfx::Image(image.release()));
}

}  // namespace api

}  // namespace atom
