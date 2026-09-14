// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRINTING_PRINTER_CAPABILITIES_QUERY_H_
#define ELECTRON_SHELL_BROWSER_PRINTING_PRINTER_CAPABILITIES_QUERY_H_

#include <string>
#include <utility>

#include "base/dcheck_is_on.h"
#include "base/functional/callback_forward.h"
#include "base/values.h"

namespace electron {

// Windows queries run in their own utility process so a blocked driver cannot
// block the browser, printer-list replies, or another capability query.
void QueryPrinterCapabilities(
    const std::string& printer_name,
    base::OnceCallback<void(std::pair<std::string, base::DictValue>)> callback);

#if DCHECK_IS_ON()
// Consumed by exactly one query. Only exposed through the existing test
// binding.
bool SetNextPrinterCapabilitiesQueryForTesting(const std::string& behavior,
                                               int timeout_ms);
#endif

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PRINTING_PRINTER_CAPABILITIES_QUERY_H_
