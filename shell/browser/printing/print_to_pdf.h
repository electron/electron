// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRINTING_PRINT_TO_PDF_H_
#define ELECTRON_SHELL_BROWSER_PRINTING_PRINT_TO_PDF_H_

#include <string_view>

#include "base/functional/callback.h"
#include "v8/include/v8-forward.h"

namespace content {
class RenderFrameHost;
}

namespace electron {

// The frame to print when a queued job starts, or null if it has gone.
using PrintToPDFFrame = base::RepeatingCallback<content::RenderFrameHost*()>;

// webContents.printToPDF(options) / webFrameMain.printToPDF(options).
// Validates |options| (which may be empty) and returns a promise for the PDF
// data as a Buffer,
// rejected with the same errors the API documents for bad options. PDF jobs in
// one frame tree conflict in the renderer, so a job waits for earlier ones with
// the same |frame_tree| key (the tree's top FrameTreeNode id) to settle, then
// prints whatever |frame| returns at that point; if that is null the promise is
// rejected with |frame_gone| (a message and whether it is a TypeError).
struct PrintToPDFFrameGone {
  std::string_view message;
  bool type_error = false;
};
v8::Local<v8::Promise> PrintToPDF(v8::Isolate* isolate,
                                  int frame_tree,
                                  PrintToPDFFrame frame,
                                  PrintToPDFFrameGone frame_gone,
                                  v8::Local<v8::Value> options);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PRINTING_PRINT_TO_PDF_H_
