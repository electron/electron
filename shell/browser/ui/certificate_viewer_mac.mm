// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/certificate_viewer.h"

#import <Cocoa/Cocoa.h>
#import <SecurityInterface/SFCertificatePanel.h>

#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_apple.h"

namespace electron {

void ShowPlatformCertificateViewer(net::X509Certificate* cert,
                                   gfx::NativeWindow parent) {
  auto cert_chain =
      net::x509_util::CreateSecCertificateArrayForX509Certificate(cert);
  if (!cert_chain)
    return;

  // Run the certificate panel app-modally. This keeps the panel's lifetime
  // scoped to this call, avoiding the manual retain dance an asynchronous
  // sheet would require.
  SFCertificatePanel* panel = [[SFCertificatePanel alloc] init];
  [panel runModalForCertificates:(__bridge NSArray*)cert_chain.get()
                       showGroup:YES];
}

}  // namespace electron
