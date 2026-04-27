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
#endif

  // content::WebAuthenticationDelegate
  bool SupportsResidentKeys(
      content::RenderFrameHost* render_frame_host) override;
#if BUILDFLAG(IS_MAC)
  std::optional<TouchIdAuthenticatorConfig> GetTouchIdAuthenticatorConfig(
      content::BrowserContext* browser_context) override;
#endif

 private:
#if BUILDFLAG(IS_MAC)
  static std::string& touch_id_keychain_access_group();
#endif
};

}  // namespace electron
#endif  // ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_DELEGATE_H_
