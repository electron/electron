// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/certificate_trust.h"

#include <memory>
#include <string>
#include <utility>

#import <Cocoa/Cocoa.h>
#import <SecurityInterface/SFCertificateTrustPanel.h>

#include "base/strings/sys_string_conversions.h"
#include "net/cert/cert_database.h"
#include "net/cert/x509_util_ios_and_mac.h"
#include "net/cert/x509_util_mac.h"
#include "shell/browser/native_window.h"

@interface TrustDelegate : NSObject {
 @private
  std::unique_ptr<gin_helper::Promise<void>> promise_;
  SFCertificateTrustPanel* panel_;
  scoped_refptr<net::X509Certificate> cert_;
  SecTrustRef trust_;
  CFArrayRef cert_chain_;
  SecPolicyRef sec_policy_;
}

- (id)initWithPromise:(gin_helper::Promise<void>)promise
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

- (id)initWithPromise:(gin_helper::Promise<void>)promise
                panel:(SFCertificateTrustPanel*)panel
                 cert:(const scoped_refptr<net::X509Certificate>&)cert
                trust:(SecTrustRef)trust
            certChain:(CFArrayRef)certChain
            secPolicy:(SecPolicyRef)secPolicy {
  if ((self = [super init])) {
    promise_ = std::make_unique<gin_helper::Promise<void>>(std::move(promise));
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
  auto* cert_db = net::CertDatabase::GetInstance();
  // This forces Chromium to reload the certificate since it might be trusted
  // now.
  cert_db->NotifyObserversCertDBChanged();

  promise_->Resolve();
  [self autorelease];
}

@end

namespace certificate_trust {

v8::Local<v8::Promise> ShowCertificateTrust(
    electron::NativeWindow* parent_window,
    const scoped_refptr<net::X509Certificate>& cert,
    const std::string& message) {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* sec_policy = SecPolicyCreateBasicX509();
  auto cert_chain =
      net::x509_util::CreateSecCertificateArrayForX509Certificate(cert.get());
  SecTrustRef trust = nullptr;
  SecTrustCreateWithCertificates(cert_chain, sec_policy, &trust);

  NSWindow* window = parent_window
                         ? parent_window->GetNativeWindow().GetNativeNSWindow()
                         : nil;
  auto msg = base::SysUTF8ToNSString(message);

  auto panel = [[SFCertificateTrustPanel alloc] init];
  auto delegate = [[TrustDelegate alloc] initWithPromise:std::move(promise)
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

  return handle;
}

}  // namespace certificate_trust
