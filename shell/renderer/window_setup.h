// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_WINDOW_SETUP_H_
#define ELECTRON_SHELL_RENDERER_WINDOW_SETUP_H_

#include "v8/include/v8-forward.h"

namespace content {
class RenderFrame;
}

namespace electron {

// Electron's per-document tweaks to the `window` object of a frame it manages:
// `window.prompt` throws (unsupported); in renderers with Node.js integration
// the top-level `window.close` closes the BrowserWindow; and a <webview>
// guest reports window focus changes so the embedder's element can emit
// focus/blur. `context` is the main-world or Electron isolated-world context
// that was just created for the frame.
void SetUpWindow(content::RenderFrame* render_frame,
                 v8::Local<v8::Context> context);

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_WINDOW_SETUP_H_
