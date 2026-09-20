// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_SECURITY_WARNINGS_H_
#define ELECTRON_SHELL_RENDERER_SECURITY_WARNINGS_H_

namespace content {
class RenderFrame;
}

namespace electron {

// Logs the "Electron Security Warning" console messages for a main frame once
// it has loaded, when running an unpackaged app (or when
// ELECTRON_ENABLE_SECURITY_WARNINGS is set). No-op otherwise.
void MaybeAddSecurityWarnings(content::RenderFrame* render_frame);

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_SECURITY_WARNINGS_H_
