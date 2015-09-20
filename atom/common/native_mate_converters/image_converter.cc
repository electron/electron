// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/image_converter.h"

#include "atom/common/api/atom_api_native_image.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "ui/gfx/image/image_skia.h"

namespace mate {

bool Converter<gfx::ImageSkia>::FromV8(v8::Isolate* isolate,
                                       v8::Local<v8::Value> val,
                                       gfx::ImageSkia* out) {
  gfx::Image image;
  if (!ConvertFromV8(isolate, val, &image))
    return false;

  *out = image.AsImageSkia();
  return true;
}

bool Converter<gfx::Image>::FromV8(v8::Isolate* isolate,
                                   v8::Local<v8::Value> val,
                                   gfx::Image* out) {
  if (val->IsNull())
    return true;

  Handle<atom::api::NativeImage> native_image;
  if (!ConvertFromV8(isolate, val, &native_image)) {
    // Try converting from file path.
    base::FilePath path;
    if (!Converter<base::FilePath>::FromV8(isolate, val, &path))
      return false;

    native_image = atom::api::NativeImage::CreateFromPath(isolate, path);
    if (native_image->image().IsEmpty())
      return false;
  }

  *out = native_image->image();
  return true;
}

v8::Local<v8::Value> Converter<gfx::Image>::ToV8(v8::Isolate* isolate,
                                                  const gfx::Image& val) {
  return ConvertToV8(isolate, atom::api::NativeImage::Create(isolate, val));
}

}  // namespace mate
