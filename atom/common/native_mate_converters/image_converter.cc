// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/image_converter.h"

#include <string>
#include <vector>

#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/file_util.h"
#include "base/strings/string_util.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/base/layout.h"

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
  { "@1.25x" , 1.25f },
  { "@1.33x" , 1.33f },
  { "@1.4x"  , 1.4f },
  { "@1.5x"  , 1.5f },
  { "@1.8x"  , 1.8f },
  { "@2.5x"  , 2.5f },
};

float GetScaleFactorFromPath(const base::FilePath& path) {
  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());

  // There is no scale info in the file path.
  if (!EndsWith(filename, "x", true))
    return 1.0f;

  // We don't try to convert string to float here because it is very very
  // expensive.
  for (unsigned i = 0; i < arraysize(kScaleFactorPairs); ++i) {
    if (EndsWith(filename, kScaleFactorPairs[i].name, true))
      return kScaleFactorPairs[i].scale;
  }

  return 1.0f;
}

void AppendIfExists(std::vector<base::FilePath>* paths,
                    const base::FilePath& path) {
  if (base::PathExists(path))
    paths->push_back(path);
}

void PopulatePossibleFilePaths(std::vector<base::FilePath>* paths,
                               const base::FilePath& path) {
  AppendIfExists(paths, path);

  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());
  if (MatchPattern(filename, "*@*x"))
    return;

  for (unsigned i = 0; i < arraysize(kScaleFactorPairs); ++i)
    AppendIfExists(paths,
                   path.InsertBeforeExtensionASCII(kScaleFactorPairs[i].name));
}

bool AddImageSkiaRepFromPath(gfx::ImageSkia* image,
                             const base::FilePath& path) {
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
    image->AddRepresentation(gfx::ImageSkiaRep(
        *decoded.release(), GetScaleFactorFromPath(path)));
    return true;
  }

  return false;
}

}  // namespace

bool Converter<gfx::ImageSkia>::FromV8(v8::Isolate* isolate,
                                       v8::Handle<v8::Value> val,
                                       gfx::ImageSkia* out) {
  base::FilePath path;
  if (Converter<base::FilePath>::FromV8(isolate, val, &path)) {
    std::vector<base::FilePath> paths;
    PopulatePossibleFilePaths(&paths, path);
    if (paths.empty())
      return false;

    for (size_t i = 0; i < paths.size(); ++i) {
      if (!AddImageSkiaRepFromPath(out, paths[i]))
        return false;
    }
    return true;
  }

  return false;
}

}  // namespace mate
