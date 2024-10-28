// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_SKIA_UTIL_H_
#define ELECTRON_SHELL_COMMON_SKIA_UTIL_H_

#include <cstdint>

#include "base/containers/span.h"

namespace base {
class FilePath;
}  // namespace base

namespace gfx {
class ImageSkia;
}

namespace electron::util {

bool PopulateImageSkiaRepsFromPath(gfx::ImageSkia* image,
                                   const base::FilePath& path);

bool AddImageSkiaRepFromBuffer(gfx::ImageSkia* image,
                               base::span<const uint8_t> data,
                               int width,
                               int height,
                               double scale_factor);

bool AddImageSkiaRepFromJPEG(gfx::ImageSkia* image,
                             base::span<const uint8_t> data,
                             double scale_factor);

bool AddImageSkiaRepFromPNG(gfx::ImageSkia* image,
                            base::span<const uint8_t> data,
                            double scale_factor);

#if BUILDFLAG(IS_WIN)
bool ReadImageSkiaFromICO(gfx::ImageSkia* image, HICON icon);
#endif

}  // namespace electron::util

#endif  // ELECTRON_SHELL_COMMON_SKIA_UTIL_H_
