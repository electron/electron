// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRINTING_PRINT_TO_PDF_H_
#define ELECTRON_SHELL_BROWSER_PRINTING_PRINT_TO_PDF_H_

#include "v8/include/v8-forward.h"

namespace base {
class Value;
}

namespace content {
class RenderFrameHost;
}

namespace electron {

// Prints the current document of |rfh| to a PDF, per the printToPDF settings
// dict built in lib/browser/print-to-pdf.ts. Returns a promise that resolves
// with the PDF data as a Buffer.
v8::Local<v8::Promise> PrintFrameToPDF(content::RenderFrameHost* rfh,
                                       const base::Value& settings);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PRINTING_PRINT_TO_PDF_H_
