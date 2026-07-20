// Copyright (c) 2022 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_DELEGATE_H_

#include <optional>
#include <string>

#include "build/build_config.h"
#include "content/public/browser/web_authentication_delegate.h"

namespace electron {

// Modified from chrome/browser/webauthn/chrome_authenticator_request_delegate.h
class ElectronWebAuthenticationDelegate
    : public content::WebAuthenticationDelegate {
 public:
  ~ElectronWebAuthenticationDelegate() override;

#if BUILDFLAG(IS_MAC)
  // Configures the keychain-access-group that the Touch ID platform
  // authenticator stores credentials under. The app's entitlements must include
  // this value in the `keychain-access-groups` array. Called from
  // `app.configureWebAuthn()`.
  static void SetTouchIdKeychainAccessGroup(std::string access_group);

  // Configures whether platform passkeys (via ASAuthorizationController) are
  // enabled. Any Credential Provider Extension (iCloud Keychain, 1Password,
  // etc.) can fulfill these requests. Called from `app.configureWebAuthn()`.
  static bool IsTouchIdConfigured();
  static void SetPlatformPasskeysEnabled(bool enabled);
  static bool IsPlatformPasskeysEnabled();
#endif

  // content::WebAuthenticationDelegate
  bool SupportsResidentKeys(
      content::RenderFrameHost* render_frame_host) override;
#if BUILDFLAG(IS_MAC)
  std::optional<TouchIdAuthenticatorConfig> GetTouchIdAuthenticatorConfig(
      content::BrowserContext* browser_context) override;

  // The default isUVPAA implementation only reports on the Touch ID platform
  // authenticator (see content/browser/webauth/is_uvpaa.cc), so an app that
  // enables only platform passkeys would report `false` and most sites would
  // hide their passkey UI. Report `true` when platform passkeys are enabled on
  // a supported macOS version so sites offer the passkey path.
  //
  // This reports capability, not configuration: it cannot cheaply verify that
  // the app is signed with an embedded provisioning profile or that the RP's
  // domain serves a valid apple-app-site-association (there is no synchronous
  // Apple API for either). A misconfigured app therefore still gets `true`
  // here, then fails the actual ceremony with ASAuthorizationError 1004 — see
  // MaybeLogUnconfiguredError in the platform authenticator.
  void IsUserVerifyingPlatformAuthenticatorAvailableOverride(
      content::RenderFrameHost* render_frame_host,
      base::OnceCallback<void(std::optional<bool>)> callback) override;
#endif

 private:
#if BUILDFLAG(IS_MAC)
  static std::string& touch_id_keychain_access_group();
  static bool& platform_passkeys_enabled();
#endif
};

}  // namespace electron
#endif  // ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_DELEGATE_H_
