// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/color_util.h"

#include "base/strings/string_util.h"

namespace atom {

SkColor ParseHexColor(const std::string& name) {
  auto color = name.substr(1);
  unsigned length = color.size();
  SkColor result = (length != 8 ? 0xFF000000 : 0x00000000);
  unsigned value = 0;
  if (length != 3 && length != 6 && length != 8)
    return result;
  for (unsigned i = 0; i < length; ++i) {
    if (!base::IsHexDigit(color[i]))
      return result;
    value <<= 4;
    value |= (color[i] < 'A' ? color[i] - '0' : (color[i] - 'A' + 10) & 0xF);
  }
  if (length == 6 || length == 8) {
    result |= value;
    return result;
  }
  result |= (value & 0xF00) << 12 | (value & 0xF00) << 8
      | (value & 0xF0) << 8 | (value & 0xF0) << 4
      | (value & 0xF) << 4 | (value & 0xF);
  return result;
}

}  // namespace atom
