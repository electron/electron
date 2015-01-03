// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/image_converter.h"

#import <Cocoa/Cocoa.h>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

namespace {

bool IsTemplateImage(const std::string& path) {
  return (MatchPattern(path, "*Template.*") ||
          MatchPattern(path, "*Template@*x.*"));
}

}  // namespace

namespace mate {

bool Converter<gfx::ImageSkia>::FromV8(v8::Isolate* isolate,
                                       v8::Handle<v8::Value> val,
                                       gfx::ImageSkia* out) {
  gfx::Image image;
  if (!ConvertFromV8(isolate, val, &image) || image.IsEmpty())
    return false;

  gfx::ImageSkia image_skia = image.AsImageSkia();
  if (image_skia.isNull())
    return false;

  *out = image_skia;
  return true;
}

bool Converter<gfx::Image>::FromV8(v8::Isolate* isolate,
                                   v8::Handle<v8::Value> val,
                                   gfx::Image* out) {
  std::string path;
  if (!ConvertFromV8(isolate, val, &path))
    return false;

  base::scoped_nsobject<NSImage> image([[NSImage alloc]
      initByReferencingFile:base::SysUTF8ToNSString(path)]);
  if (![image isValid])
    return false;

  if (IsTemplateImage(path))
    [image setTemplate:YES];

  *out = gfx::Image(image.release());
  return true;
}

}  // namespace mate
