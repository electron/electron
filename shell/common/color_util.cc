// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/color_util.h"

#include <cmath>
#include <vector>

#include "base/cxx17_backports.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/color_parser.h"
#include "ui/gfx/color_utils.h"

namespace {

bool IsHexFormat(const std::string& str) {
  // Must be either #ARGB or #AARRGGBB.
  bool is_hex_length = str.length() == 5 || str.length() == 9;
  if (str[0] != '#' || !is_hex_length)
    return false;

  if (!std::all_of(str.begin() + 1, str.end(), ::isxdigit))
    return false;

  return true;
}

}  // namespace

namespace electron {

SkColor ParseCSSColor(const std::string& color_string) {
  // ParseCssColorString expects RGBA and we historically use ARGB
  // so we need to convert before passing to ParseCssColorString.
  std::string color_str = color_string;
  if (IsHexFormat(color_str)) {
    if (color_str.length() == 5) {
      // #ARGB => #RGBA
      std::swap(color_str[1], color_str[4]);
    } else {
      // #AARRGGBB => #RRGGBBAA
      std::swap(color_str[1], color_str[7]);
      std::swap(color_str[2], color_str[8]);
    }
  }

  SkColor color;
  if (!content::ParseCssColorString(color_str, &color))
    color = SK_ColorWHITE;

  return color;
}

std::string ToRGBHex(SkColor color) {
  return base::StringPrintf("#%02X%02X%02X", SkColorGetR(color),
                            SkColorGetG(color), SkColorGetB(color));
}

std::string ToRGBAHex(SkColor color, bool include_hash) {
  std::string color_str = base::StringPrintf(
      "%02X%02X%02X%02X", SkColorGetR(color), SkColorGetG(color),
      SkColorGetB(color), SkColorGetA(color));
  if (include_hash) {
    return "#" + color_str;
  }
  return color_str;
}

}  // namespace electron
