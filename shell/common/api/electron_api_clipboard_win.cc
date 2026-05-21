// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_clipboard.h"

#include <windows.h>  // NOLINT(build/include_order)

#include <cstdint>
#include <string>
#include <string_view>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"

namespace electron::api::clipboard_util {

std::string ResolvePlatformFormatName(const ui::ClipboardFormatType& fmt) {
  // On Windows `ClipboardFormatType::GetName()` returns the numeric
  // registered-format id (see `clipboard_format_type_win.cc` — `GetName()`
  // just forwards to `Serialize()`). Resolve it back to the registered
  // string name so a raw `electron application/osclipboard;format="..."`
  // format round-trips through the same MIME on write and read.
  uint32_t cf_format = 0;
  if (!base::StringToUint(fmt.GetName(), &cf_format))
    return fmt.GetName();

  // Registered formats live in the 0xC000-0xFFFF range and have a name.
  // Predefined formats (e.g. `CF_TEXT`) return 0 from
  // `GetClipboardFormatName` — fall back to the numeric id for those.
  constexpr int kMaxFormatNameLen = 256;
  wchar_t name_buf[kMaxFormatNameLen];
  const int len = ::GetClipboardFormatNameW(static_cast<UINT>(cf_format),
                                            name_buf, kMaxFormatNameLen);
  if (len <= 0)
    return fmt.GetName();

  return base::WideToUTF8(std::wstring_view(name_buf, len));
}

}  // namespace electron::api::clipboard_util
