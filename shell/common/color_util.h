// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_COLOR_UTIL_H_
#define ELECTRON_SHELL_COMMON_COLOR_UTIL_H_

#include <string>

#include "third_party/skia/include/core/SkColor.h"

// SkColor is a typedef for uint32_t, this wrapper is to tag an SkColor for
// ease of use in gin converters.
struct WrappedSkColor {
  WrappedSkColor() {}
  WrappedSkColor(SkColor c) : value(c) {}  // NOLINT(runtime/explicit)
  SkColor value;
  operator SkColor() const { return value; }
};

namespace electron {

// Parses a CSS-style color string from hex, rgb(), rgba(),
// hsl(), hsla(), or color name formats.
SkColor ParseCSSColor(const std::string& color_string);

// Convert color to RGB hex value like "#RRGGBB".
std::string ToRGBHex(SkColor color);

// Convert color to RGBA hex value like "#RRGGBBAA".
std::string ToRGBAHex(SkColor color, bool include_hash = true);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_COLOR_UTIL_H_
