// Copyright (c) 2024 Microsoft, GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRINTING_PRINTING_UTILS_H_
#define ELECTRON_SHELL_BROWSER_PRINTING_PRINTING_UTILS_H_

#include <string>

#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "printing/backend/print_backend.h"
#include "ui/gfx/geometry/size.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace electron {

// The PDF plugin frame for the PDF viewer, the focused frame if it has a
// selection, otherwise the primary main frame.
content::RenderFrameHost* GetRenderFrameHostToUse(
    content::WebContents* contents);

struct ResolvedPrinter {
  ResolvedPrinter();
  ResolvedPrinter(ResolvedPrinter&&);
  ResolvedPrinter& operator=(ResolvedPrinter&&);
  ~ResolvedPrinter();

  std::string name;
  // Empty unless capabilities were requested and reported.
  gfx::Size dpi;
  gfx::Size default_paper_um;
};

using ResolvePrinterCallback =
    base::OnceCallback<void(base::expected<ResolvedPrinter, std::string>)>;

// Looks up `device_name`, or the default printer when empty; `want_caps` also
// fetches its DPI and default paper size. Replies on the UI thread.
void ResolvePrinter(const std::string& device_name,
                    bool want_caps,
                    ResolvePrinterCallback callback);

void GetPrinterList(base::OnceCallback<void(printing::PrinterList)> callback);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PRINTING_PRINTING_UTILS_H_
