// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#import "chrome/browser/ui/cocoa/certificate_viewer_mac.h"
#import "base/mac/scoped_nsobject.h"

#include "chrome/browser/certificate_viewer.h"

@interface SSLCertificateViewerMacViews : SSLCertificateViewerMac {}
@end

class CertificateViewerDelegate {
  public:
    CertificateViewerDelegate(content::WebContents* web_contents,
        gfx::NativeWindow parent,
        net::X509Certificate* cert)
        : certificate_viewer_([[SSLCertificateViewerMacViews alloc]
              initWithCertificate:cert
                  forWebContents:web_contents]) {
      [certificate_viewer_ showCertificateSheet:parent];
    }

  private:
    base::scoped_nsobject<SSLCertificateViewerMacViews> certificate_viewer_;

  DISALLOW_COPY_AND_ASSIGN(CertificateViewerDelegate);
};

@implementation SSLCertificateViewerMacViews

- (void)sheetDidEnd:(NSWindow*)parent
         returnCode:(NSInteger)returnCode
            context:(void*)context {
  [self closeCertificateSheet];
  [self releaseSheetWindow];
}

@end

void ShowCertificateViewer(content::WebContents* web_contents,
                           gfx::NativeWindow parent,
                           net::X509Certificate* cert) {
  // Shows a new widget, which owns the delegate.
  new CertificateViewerDelegate(web_contents, parent, cert);
}