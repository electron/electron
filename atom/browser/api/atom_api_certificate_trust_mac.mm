// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_certificate_trust.h"

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <SecurityInterface/SFCertificateTrustPanel.h>

#include "atom/browser/native_window.h"
#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/sys_string_conversions.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/cert_database.h"

@interface Trampoline : NSObject

- (void)createPanelDidEnd:(NSWindow *)sheet
        returnCode:(int)returnCode
        contextInfo:(void *)contextInfo;

@end

@implementation Trampoline

- (void)createPanelDidEnd:(NSWindow *)sheet
        returnCode:(int)returnCode
        contextInfo:(void *)contextInfo {
  void (^block)(int) = (void (^)(int))contextInfo;
  block(returnCode);
  [(id)block autorelease];
}

@end

namespace atom {

namespace api {

void ShowCertificateTrustUI(atom::NativeWindow* parent_window,
                            const scoped_refptr<net::X509Certificate>& cert,
                            std::string message,
                            const ShowTrustCallback& callback) {
  auto sec_policy = SecPolicyCreateBasicX509();
  auto cert_chain = cert->CreateOSCertChainForCert();
  SecTrustRef trust = nullptr;
  SecTrustCreateWithCertificates(cert_chain, sec_policy, &trust);

  SFCertificateTrustPanel *panel = [[SFCertificateTrustPanel alloc] init];

  void (^callbackBlock)(int) = [^(int returnCode) {
    // if (returnCode == NSFileHandlingPanelOKButton) {
      auto cert_db = net::CertDatabase::GetInstance();
      // This forces Chromium to reload the certificate since it might be trusted
      // now.
      cert_db->NotifyObserversCertDBChanged(cert.get());
    // }

    callback.Run(returnCode);

    [panel autorelease];
    CFRelease(trust);
    CFRelease(cert_chain);
    CFRelease(sec_policy);
  } copy];

  NSWindow* window = parent_window ?
      parent_window->GetNativeWindow() :
      NULL;
  auto msg = base::SysUTF8ToNSString(message);
  [panel beginSheetForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:callbackBlock trust:trust message:msg];
}

}  // namespace api

}  // namespace atom
