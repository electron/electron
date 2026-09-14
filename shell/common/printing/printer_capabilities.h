// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_PRINTING_PRINTER_CAPABILITIES_H_
#define ELECTRON_SHELL_COMMON_PRINTING_PRINTER_CAPABILITIES_H_

#include <string>
#include <string_view>
#include <utility>

#include "base/values.h"

namespace electron {

// Checks the syntax of the IDs shared by the public API and capability lists.
bool IsPrinterMediaIdValid(std::string_view id);

// Returns <error, capabilities>. This can block inside an OS printer driver.
// Windows callers must use an isolated utility process, never the browser UI.
std::pair<std::string, base::DictValue> GetPrinterCapabilities(
    const std::string& printer_name);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_PRINTING_PRINTER_CAPABILITIES_H_
