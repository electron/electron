// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/image_converter.h"

#include <string>

#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/file_util.h"
#include "base/strings/string_util.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/base/layout.h"

namespace mate {

namespace {

ui::ScaleFactor GetScaleFactorFromFileName(const base::FilePath& path) {
  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());
  if (EndsWith(filename, "@2x", true))
    return ui::SCALE_FACTOR_200P;
  else
    return ui::SCALE_FACTOR_100P;
}

}  // namespace

bool Converter<gfx::ImageSkia>::FromV8(v8::Isolate* isolate,
                                       v8::Handle<v8::Value> val,
                                       gfx::ImageSkia* out) {
  base::FilePath path;
  if (Converter<base::FilePath>::FromV8(isolate, val, &path)) {
    std::string file_contents;
    if (!base::ReadFileToString(path, &file_contents))
      return false;

    const unsigned char* data =
        reinterpret_cast<const unsigned char*>(file_contents.data());
    size_t size = file_contents.size();
    scoped_ptr<SkBitmap> decoded(new SkBitmap());

    // Try PNG first.
    if (!gfx::PNGCodec::Decode(data, size, decoded.get()))
      // Try JPEG.
      decoded.reset(gfx::JPEGCodec::Decode(data, size));

    if (decoded) {
      *out = gfx::ImageSkia(gfx::ImageSkiaRep(*decoded.release(),
                            GetScaleFactorFromFileName(path)));
      return true;
    }
  }

  return false;
}

}  // namespace mate
