// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/color_util.h"

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace atom {

SkColor ParseHexColor(const std::string& color_string) {
  // Check the string for incorrect formatting.
  if (color_string.empty() || color_string[0] != '#')
    return SK_ColorWHITE;

  // Prepend FF if alpha channel is not specified.
  std::string source = color_string.substr(1);
  if (source.size() == 3)
    source.insert(0, "F");
  else if (source.size() == 6)
    source.insert(0, "FF");

  // Convert the string from #FFF format to #FFFFFF format.
  std::string formatted_color;
  if (source.size() == 4) {
    for (size_t i = 0; i < 4; ++i) {
      formatted_color += source[i];
      formatted_color += source[i];
    }
  } else if (source.size() == 8) {
    formatted_color = source;
  } else {
    return SK_ColorWHITE;
  }

  // Convert the string to an integer and make sure it is in the correct value
  // range.
  std::vector<uint8_t> bytes;
  if (!base::HexStringToBytes(formatted_color, &bytes))
    return SK_ColorWHITE;

  return SkColorSetARGB(bytes[0], bytes[1], bytes[2], bytes[3]);
}

std::string ToRGBHex(SkColor color) {
  return base::StringPrintf("#%02X%02X%02X",
                            SkColorGetR(color),
                            SkColorGetG(color),
                            SkColorGetB(color));
}

}  // namespace atom
