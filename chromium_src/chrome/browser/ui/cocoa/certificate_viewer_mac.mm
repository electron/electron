// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/certificate_viewer_mac.h"

#import <objc/runtime.h>

#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#import "base/mac/scoped_nsobject.h"
#include "content/public/browser/web_contents.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_ios_and_mac.h"
#include "net/cert/x509_util_mac.h"

namespace {

// The maximum height of the certificate panel. Imposed whenever Sierra's buggy
// autolayout algorithm tries to change it. Doesn't affect user resizes.
const CGFloat kMaxPanelSetFrameHeight = 400;

// Pointer to the real implementation of -[SFCertificatePanel setFrame:..]. This
// is cached so a correct lookup is performed before we add the override.
IMP g_real_certificate_panel_setframe = nullptr;

// Provide a workaround for a buggy autolayout algorithm in macOS Sierra when
// running Chrome linked against the 10.10 SDK on macOS 10.12.
// See http://crbug.com/643123 for more details.
// Note it's not possible to inherit from SFCertificatePanel without triggering
// *** Assertion failure in -[BrowserCrApplication
// _commonBeginModalSessionForWindow:relativeToWindow:modalDelegate:
// didEndSelector:contextInfo:], .../AppKit.subproj/NSApplication.m:4077
// After that assertion, the sheet simply refuses to continue loading.
// It's also not possible to swizzle the -setFrame: method because
// SFCertificatePanel doesn't define it. Attempting to swizzle that would
// replace the implementation on NSWindow and constrain all dialogs.
// So, provide a regular C method and append it to the SFCertificatePanel
// implementation using the objc runtime.
// TODO(tapted): Remove all of this when Chrome's SDK target gets bumped.
id SFCertificatePanelSetFrameOverride(id self,
                                      SEL _cmd,
                                      NSRect frame,
                                      BOOL display,
                                      BOOL animate) {
  if (frame.size.height > kMaxPanelSetFrameHeight)
    frame.size.height = kMaxPanelSetFrameHeight;

  DCHECK(g_real_certificate_panel_setframe);
  return g_real_certificate_panel_setframe(self, _cmd, frame, display, animate);
}

void MaybeConstrainPanelSizeForSierraBug() {
#if defined(MAC_OS_X_VERSION_10_11) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_11
  // This is known not to be required when linking against the 10.11 SDK. Early
  // exit in that case but assume anything earlier is broken.
  return;
#endif

  // It's also not required when running on El Capitan or earlier.
  if (base::mac::IsAtMostOS10_11() || g_real_certificate_panel_setframe)
    return;

  const SEL kSetFrame = @selector(setFrame:display:animate:);
  Method real_method =
      class_getInstanceMethod([SFCertificatePanel class], kSetFrame);
  const char* type_encoding = method_getTypeEncoding(real_method);
  g_real_certificate_panel_setframe = method_getImplementation(real_method);
  DCHECK(g_real_certificate_panel_setframe);
  IMP method = reinterpret_cast<IMP>(&SFCertificatePanelSetFrameOverride);
  BOOL method_added = class_addMethod([SFCertificatePanel class], kSetFrame,
                                      method, type_encoding);
  DCHECK(method_added);
}

}  // namespace

@interface SFCertificatePanel (SystemPrivate)
// A system-private interface that dismisses a panel whose sheet was started by
// -beginSheetForWindow:
//        modalDelegate:
//       didEndSelector:
//          contextInfo:
//         certificates:
//            showGroup:
// as though the user clicked the button identified by returnCode. Verified
// present in 10.8.
- (void)_dismissWithCode:(NSInteger)code;
@end

@implementation SSLCertificateViewerMac {
  // The corresponding list of certificates.
  base::scoped_nsobject<NSArray> certificates_;
  base::scoped_nsobject<SFCertificatePanel> panel_;
}

- (instancetype)initWithCertificate:(net::X509Certificate*)certificate
                     forWebContents:(content::WebContents*)webContents {
  if ((self = [super init])) {
    base::ScopedCFTypeRef<CFArrayRef> certChain(
        net::x509_util::CreateSecCertificateArrayForX509Certificate(
            certificate));
    if (!certChain)
      return self;
    NSArray* certificates = base::mac::CFToNSCast(certChain.get());
    certificates_.reset([certificates retain]);
  }

  // Explicitly disable revocation checking, regardless of user preferences
  // or system settings. The behaviour of SFCertificatePanel is to call
  // SecTrustEvaluate on the certificate(s) supplied, effectively
  // duplicating the behaviour of net::X509Certificate::Verify(). However,
  // this call stalls the UI if revocation checking is enabled in the
  // Keychain preferences or if the cert may be an EV cert. By disabling
  // revocation checking, the stall is limited to the time taken for path
  // building and verification, which should be minimized due to the path
  // being provided in |certificates|. This does not affect normal
  // revocation checking from happening, which is controlled by
  // net::X509Certificate::Verify() and user preferences, but will prevent
  // the certificate viewer UI from displaying which certificate is revoked.
  // This is acceptable, as certificate revocation will still be shown in
  // the page info bubble if a certificate in the chain is actually revoked.
  base::ScopedCFTypeRef<CFMutableArrayRef> policies(
      CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks));
  if (!policies.get()) {
    NOTREACHED();
    return self;
  }
  // Add a basic X.509 policy, in order to match the behaviour of
  // SFCertificatePanel when no policies are specified.
  SecPolicyRef basicPolicy = nil;
  OSStatus status = net::x509_util::CreateBasicX509Policy(&basicPolicy);
  if (status != noErr) {
    NOTREACHED();
    return self;
  }
  CFArrayAppendValue(policies, basicPolicy);
  CFRelease(basicPolicy);

  status = net::x509_util::CreateRevocationPolicies(false, policies);
  if (status != noErr) {
    NOTREACHED();
    return self;
  }

  panel_.reset([[SFCertificatePanel alloc] init]);
  [panel_ setPolicies:base::mac::CFToNSCast(policies.get())];
  return self;
}

- (void)sheetDidEnd:(NSWindow*)parent
         returnCode:(NSInteger)returnCode
            context:(void*)context {
  NOTREACHED();  // Subclasses must implement this.
}

- (void)showCertificateSheet:(NSWindow*)window {
  MaybeConstrainPanelSizeForSierraBug();

  [panel_ beginSheetForWindow:window
                modalDelegate:self
               didEndSelector:@selector(sheetDidEnd:returnCode:context:)
                  contextInfo:nil
                 certificates:certificates_
                    showGroup:YES];
}

- (void)closeCertificateSheet {
  // Closing the sheet using -[NSApp endSheet:] doesn't work so use the private
  // method.
  [panel_ _dismissWithCode:NSFileHandlingPanelCancelButton];
  certificates_.reset();
}

- (void)releaseSheetWindow {
  panel_.reset();
}

- (NSWindow*)certificatePanel {
  return panel_;
}

@end
