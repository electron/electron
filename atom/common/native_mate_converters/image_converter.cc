// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/image_converter.h"

#include <string>
#include <vector>

#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/base/layout.h"

#if !defined(OS_MACOSX)

namespace mate {

namespace {

struct ScaleFactorPair {
  const char* name;
  float scale;
};

ScaleFactorPair kScaleFactorPairs[] = {
  // The "@2x" is put as first one to make scale matching faster.
  { "@2x"    , 2.0f },
  { "@3x"    , 3.0f },
  { "@1x"    , 1.0f },
  { "@4x"    , 4.0f },
  { "@5x"    , 5.0f },
  { "@1.25x" , 1.25f },
  { "@1.33x" , 1.33f },
  { "@1.4x"  , 1.4f },
  { "@1.5x"  , 1.5f },
  { "@1.8x"  , 1.8f },
  { "@2.5x"  , 2.5f },
};

float GetScaleFactorFromPath(const base::FilePath& path) {
  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());

  // We don't try to convert string to float here because it is very very
  // expensive.
  for (unsigned i = 0; i < arraysize(kScaleFactorPairs); ++i) {
    if (EndsWith(filename, kScaleFactorPairs[i].name, true))
      return kScaleFactorPairs[i].scale;
  }

  return 1.0f;
}

bool AddImageSkiaRep(gfx::ImageSkia* image,
                     const base::FilePath& path,
                     double scale_factor) {
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

  if (!decoded)
    return false;

  image->AddRepresentation(gfx::ImageSkiaRep(*decoded.release(), scale_factor));
  return true;
}

bool PopulateImageSkiaRepsFromPath(gfx::ImageSkia* image,
                                   const base::FilePath& path) {
  bool succeed = false;
  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());
  if (MatchPattern(filename, "*@*x"))
    // Don't search for other representations if the DPI has been specified.
    return AddImageSkiaRep(image, path, GetScaleFactorFromPath(path));
  else
    succeed |= AddImageSkiaRep(image, path, 1.0f);

  for (const ScaleFactorPair& pair : kScaleFactorPairs)
    succeed |= AddImageSkiaRep(image,
                               path.InsertBeforeExtensionASCII(pair.name),
                               pair.scale);
  return succeed;
}

}  // namespace

bool Converter<gfx::ImageSkia>::FromV8(v8::Isolate* isolate,
                                       v8::Handle<v8::Value> val,
                                       gfx::ImageSkia* out) {
  if (val->IsNull())
    return true;

  base::FilePath path;
  if (!Converter<base::FilePath>::FromV8(isolate, val, &path))
    return false;

  return PopulateImageSkiaRepsFromPath(out, path);
}

bool Converter<gfx::Image>::FromV8(v8::Isolate* isolate,
                                   v8::Handle<v8::Value> val,
                                   gfx::Image* out) {
  gfx::ImageSkia image;
  if (!ConvertFromV8(isolate, val, &image))
    return false;

  *out = gfx::Image(image);
  return true;
}

}  // namespace mate

#endif  // !defined(OS_MACOSX)
