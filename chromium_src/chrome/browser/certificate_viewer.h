// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CERTIFICATE_VIEWER_H_
#define CHROME_BROWSER_CERTIFICATE_VIEWER_H_

#include "ui/gfx/native_widget_types.h"

namespace content {
class WebContents;
}

namespace net {
class X509Certificate;
}
// Opens a certificate viewer under |parent| to display |cert|.
void ShowCertificateViewer(content::WebContents* web_contents,
                           gfx::NativeWindow parent,
                           net::X509Certificate* cert);

#endif  // CHROME_BROWSER_CERTIFICATE_VIEWER_H_
