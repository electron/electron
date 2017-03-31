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

namespace atom {

namespace api {

void ShowCertificateTrustUI(atom::NativeWindow* parent_window,
                            const scoped_refptr<net::X509Certificate>& cert,
                            std::string message,
                            const ShowTrustCallback& callback) {
  auto sec_policy = SecPolicyCreateBasicX509();
  SecTrustRef trust = nullptr;
  SecTrustCreateWithCertificates(cert->CreateOSCertChainForCert(), sec_policy, &trust);
  // CFRelease(sec_policy);

  NSWindow* window = parent_window ?
      parent_window->GetNativeWindow() :
      NULL;

  auto msg = base::SysUTF8ToNSString(message);

  SFCertificateTrustPanel *panel = [[SFCertificateTrustPanel alloc] init];
  [panel beginSheetForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL trust:trust message:msg];

  callback.Run(true);
  // CFRelease(trust);
  // [panel release];
}

}  // namespace api

}  // namespace atom
