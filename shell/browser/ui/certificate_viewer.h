// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_CERTIFICATE_VIEWER_H_
#define ELECTRON_SHELL_BROWSER_UI_CERTIFICATE_VIEWER_H_

#include "ui/gfx/native_ui_types.h"

namespace net {
class X509Certificate;
}  // namespace net

namespace electron {

// Shows the platform's native (read-only) certificate viewer for |cert|,
// parented to |parent| when possible. Only implemented on macOS and Windows;
// callers must guard call sites accordingly.
void ShowPlatformCertificateViewer(net::X509Certificate* cert,
                                   gfx::NativeWindow parent);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_CERTIFICATE_VIEWER_H_
