// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/color_util.h"

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/color_parser.h"

namespace electron {

SkColor ParseCSSColor(const std::string& color_string) {
  SkColor color;
  if (!content::ParseCssColorString(color_string, &color))
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
