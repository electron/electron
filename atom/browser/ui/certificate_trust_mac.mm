// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/certificate_trust.h"

#import <Cocoa/Cocoa.h>
#import <SecurityInterface/SFCertificateTrustPanel.h>

#include "atom/browser/native_window.h"
#include "base/strings/sys_string_conversions.h"
#include "net/cert/cert_database.h"
#include "net/cert/x509_util_ios_and_mac.h"
#include "net/cert/x509_util_mac.h"

@interface TrustDelegate : NSObject {
 @private
  certificate_trust::ShowTrustCallback callback_;
  SFCertificateTrustPanel* panel_;
  scoped_refptr<net::X509Certificate> cert_;
  SecTrustRef trust_;
  CFArrayRef cert_chain_;
  SecPolicyRef sec_policy_;
}

- (id)initWithCallback:(const certificate_trust::ShowTrustCallback&)callback
      panel:(SFCertificateTrustPanel*)panel
      cert:(const scoped_refptr<net::X509Certificate>&)cert
      trust:(SecTrustRef)trust
      certChain:(CFArrayRef)certChain
      secPolicy:(SecPolicyRef)secPolicy;

- (void)panelDidEnd:(NSWindow*)sheet
        returnCode:(int)returnCode
        contextInfo:(void*)contextInfo;

@end

@implementation TrustDelegate

- (void)dealloc {
  [panel_ release];
  CFRelease(trust_);
  CFRelease(cert_chain_);
  CFRelease(sec_policy_);

  [super dealloc];
}

- (id)initWithCallback:(const certificate_trust::ShowTrustCallback&)callback
      panel:(SFCertificateTrustPanel*)panel
      cert:(const scoped_refptr<net::X509Certificate>&)cert
      trust:(SecTrustRef)trust
      certChain:(CFArrayRef)certChain
      secPolicy:(SecPolicyRef)secPolicy {
  if ((self = [super init])) {
    callback_ = callback;
    panel_ = panel;
    cert_ = cert;
    trust_ = trust;
    cert_chain_ = certChain;
    sec_policy_ = secPolicy;
  }

  return self;
}

- (void)panelDidEnd:(NSWindow*)sheet
        returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  auto cert_db = net::CertDatabase::GetInstance();
  // This forces Chromium to reload the certificate since it might be trusted
  // now.
  cert_db->NotifyObserversCertDBChanged();

  callback_.Run();

  [self autorelease];
}

@end

namespace certificate_trust {

void ShowCertificateTrust(atom::NativeWindow* parent_window,
                          const scoped_refptr<net::X509Certificate>& cert,
                          const std::string& message,
                          const ShowTrustCallback& callback) {
  auto sec_policy = SecPolicyCreateBasicX509();
  auto cert_chain =
      net::x509_util::CreateSecCertificateArrayForX509Certificate(cert.get());
  SecTrustRef trust = nullptr;
  SecTrustCreateWithCertificates(cert_chain, sec_policy, &trust);

  NSWindow* window = parent_window ?
      parent_window->GetNativeWindow() :
      nil;
  auto msg = base::SysUTF8ToNSString(message);

  auto panel = [[SFCertificateTrustPanel alloc] init];
  auto delegate = [[TrustDelegate alloc] initWithCallback:callback
                                         panel:panel
                                         cert:cert
                                         trust:trust
                                         certChain:cert_chain
                                         secPolicy:sec_policy];
  [panel beginSheetForWindow:window
         modalDelegate:delegate
         didEndSelector:@selector(panelDidEnd:returnCode:contextInfo:)
         contextInfo:nil
         trust:trust
         message:msg];
}

}  // namespace certificate_trust
