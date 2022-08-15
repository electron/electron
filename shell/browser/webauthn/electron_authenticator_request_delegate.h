// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_WEBAUTHN_CHROME_AUTHENTICATOR_REQUEST_DELEGATE_H_
#define ELECTRON_BROWSER_WEBAUTHN_CHROME_AUTHENTICATOR_REQUEST_DELEGATE_H_

#include "content/public/browser/authenticator_request_client_delegate.h"

namespace electron {
// ElectronWebAuthenticationDelegate is the //Electron layer implementation of
// content::WebAuthenticationDelegate.
class ElectronWebAuthenticationDelegate
    : public content::WebAuthenticationDelegate {
 public:
#if BUILDFLAG(IS_MAC)
  // Returns a configuration struct for instantiating the macOS WebAuthn
  // platform authenticator for the given Profile.
  static TouchIdAuthenticatorConfig TouchIdAuthenticatorConfigForProfile(
      Profile* profile);
#endif  // BUILDFLAG(IS_MAC)

  ~ElectronWebAuthenticationDelegate() override;

#if !BUILDFLAG(IS_ANDROID)
  // content::WebAuthenticationDelegate:
  // bool OverrideCallerOriginAndRelyingPartyIdValidation(
  //    content::BrowserContext* browser_context,
  //    const url::Origin& caller_origin,
  //    const std::string& relying_party_id) override;
  // bool OriginMayUseRemoteDesktopClientOverride(
  //    content::BrowserContext* browser_context,
  //    const url::Origin& caller_origin) override;
  // absl::optional<std::string> MaybeGetRelyingPartyIdOverride(
  //    const std::string& claimed_relying_party_id,
  //    const url::Origin& caller_origin) override;
  // bool ShouldPermitIndividualAttestation(
  //    content::BrowserContext* browser_context,
  //    const url::Origin& caller_origin,
  //    const std::string& relying_party_id) override;
  bool SupportsResidentKeys(
      content::RenderFrameHost* render_frame_host) override;
  // bool IsFocused(content::WebContents* web_contents) override;
  // absl::optional<bool> IsUserVerifyingPlatformAuthenticatorAvailableOverride(
  //     content::RenderFrameHost* render_frame_host) override;
  // content::WebAuthenticationRequestProxy* MaybeGetRequestProxy(
  //     content::BrowserContext* browser_context) override;
#endif
  // #if BUILDFLAG(IS_WIN)
  //   void OperationSucceeded(content::BrowserContext* browser_context,
  //                           bool used_win_api) override;
  // #endif
  // #if BUILDFLAG(IS_MAC)
  //   absl::optional<TouchIdAuthenticatorConfig> GetTouchIdAuthenticatorConfig(
  //       content::BrowserContext* browser_context) override;
  // #endif  // BUILDFLAG(IS_MAC)
  // #if BUILDFLAG(IS_CHROMEOS)
  //   ChromeOSGenerateRequestIdCallback GetGenerateRequestIdCallback(
  //       content::RenderFrameHost* render_frame_host) override;
  // #endif  // BUILDFLAG(IS_CHROMEOS)
};

}  // namespace electron
#endif