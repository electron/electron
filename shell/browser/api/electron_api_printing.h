// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PRINTING_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PRINTING_H_

#include "printing/buildflags/buildflags.h"
#include "v8/include/v8-forward.h"

namespace electron::api {

#if BUILDFLAG(ENABLE_PRINTING)
// Resolves with the system's printers (webContents.getPrintersAsync()).
v8::Local<v8::Promise> GetPrinterListAsync(v8::Isolate* isolate);
#endif

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PRINTING_H_
