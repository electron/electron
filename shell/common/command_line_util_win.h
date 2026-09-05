// Copyright (c) 2026 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_COMMAND_LINE_UTIL_WIN_H_
#define ELECTRON_SHELL_COMMON_COMMAND_LINE_UTIL_WIN_H_

#include <string>

namespace electron {

// Quotes |arg| using CommandLineToArgvW-compatible quoting rules so that
// the argument round-trips correctly through CreateProcess →
// CommandLineToArgvW. If no quoting is necessary the string is returned
// unchanged. See http://msdn.microsoft.com/en-us/library/17w5ykft.aspx
std::wstring AddQuoteForArg(const std::wstring& arg);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_COMMAND_LINE_UTIL_WIN_H_
