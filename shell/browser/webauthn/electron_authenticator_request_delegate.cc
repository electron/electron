// Copyright (c) 2022 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/webauthn/electron_authenticator_request_delegate.h"

#if BUILDFLAG(IS_MAC)
#include "base/base64.h"
#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "components/prefs/pref_service.h"
#include "device/fido/mac/credential_metadata.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/common/electron_constants.h"
#endif

namespace electron {

ElectronWebAuthenticationDelegate::~ElectronWebAuthenticationDelegate() =
    default;

bool ElectronWebAuthenticationDelegate::SupportsResidentKeys(
    content::RenderFrameHost* render_frame_host) {
  return true;
}

#if BUILDFLAG(IS_MAC)
// static
std::string&
ElectronWebAuthenticationDelegate::touch_id_keychain_access_group() {
  static base::NoDestructor<std::string> value;
  return *value;
}

// static
void ElectronWebAuthenticationDelegate::SetTouchIdKeychainAccessGroup(
    std::string access_group) {
  touch_id_keychain_access_group() = std::move(access_group);
}

std::optional<content::WebAuthenticationDelegate::TouchIdAuthenticatorConfig>
ElectronWebAuthenticationDelegate::GetTouchIdAuthenticatorConfig(
    content::BrowserContext* browser_context) {
  const std::string& access_group = touch_id_keychain_access_group();
  if (access_group.empty()) {
    return std::nullopt;
  }

  PrefService* prefs =
      static_cast<ElectronBrowserContext*>(browser_context)->prefs();
  std::string metadata_secret =
      prefs->GetString(kWebAuthnTouchIdMetadataSecretPrefName);
  if (metadata_secret.empty() ||
      !base::Base64Decode(metadata_secret, &metadata_secret)) {
    metadata_secret = device::fido::mac::GenerateCredentialMetadataSecret();
    prefs->SetString(kWebAuthnTouchIdMetadataSecretPrefName,
                     base::Base64Encode(base::as_byte_span(metadata_secret)));
  }

  return TouchIdAuthenticatorConfig{
      .keychain_access_group = access_group,
      .metadata_secret = std::move(metadata_secret)};
}
#endif  // BUILDFLAG(IS_MAC)

}  // namespace electron
