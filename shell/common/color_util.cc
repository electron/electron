// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/color_util.h"

#include <algorithm>
#include <cmath>

#include "content/public/common/color_parser.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

#if BUILDFLAG(IS_WIN)
#include <dwmapi.h>

#include "base/win/registry.h"
#include "skia/ext/skia_utils_win.h"
#endif

namespace {

bool IsHexFormatWithAlpha(const std::string& str) {
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

std::optional<SkColor> ParseCSSColor(const std::string& color_string) {
  // ParseCssColorString expects RGBA and we historically use ARGB
  // so we need to convert before passing to ParseCssColorString.
  std::string converted_color_str;
  if (IsHexFormatWithAlpha(color_string)) {
    std::string temp = color_string;
    int len = color_string.length() == 5 ? 1 : 2;
    converted_color_str = temp.erase(1, len) + color_string.substr(1, len);
  } else {
    converted_color_str = color_string;
  }

  SkColor color;
  if (!content::ParseCssColorString(converted_color_str, &color))
    return std::nullopt;

  return color;
}

std::string ToRGBHex(SkColor color) {
  return absl::StrFormat("#%02X%02X%02X", SkColorGetR(color),
                         SkColorGetG(color), SkColorGetB(color));
}

std::string ToRGBAHex(SkColor color, bool include_hash) {
  std::string color_str = absl::StrFormat(
      "%02X%02X%02X%02X", SkColorGetR(color), SkColorGetG(color),
      SkColorGetB(color), SkColorGetA(color));
  if (include_hash) {
    return "#" + color_str;
  }
  return color_str;
}

#if BUILDFLAG(IS_WIN)
std::optional<DWORD> GetSystemAccentColor() {
  base::win::RegKey key;
  if (key.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\DWM",
               KEY_READ) != ERROR_SUCCESS) {
    return std::nullopt;
  }

  DWORD accent_color = 0;
  if (key.ReadValueDW(L"AccentColor", &accent_color) != ERROR_SUCCESS) {
    return std::nullopt;
  }

  return accent_color;
}

SkColor GetSysSkColor(int which) {
  return skia::COLORREFToSkColor(GetSysColor(which));
}
#endif

}  // namespace electron
