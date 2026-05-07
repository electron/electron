// Copyright (c) 2026 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_PLATFORM_PASSKEYS_AUTHENTICATOR_H_
#define ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_PLATFORM_PASSKEYS_AUTHENTICATOR_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/global_routing_id.h"
#include "device/fido/fido_authenticator.h"
#include "device/fido/public/fido_transport_protocol.h"

namespace electron {

class ElectronPlatformPasskeysAuthenticator : public device::FidoAuthenticator {
 public:
  explicit ElectronPlatformPasskeysAuthenticator(
      content::GlobalRenderFrameHostToken render_frame_host_token);
  ~ElectronPlatformPasskeysAuthenticator() override;

  ElectronPlatformPasskeysAuthenticator(
      const ElectronPlatformPasskeysAuthenticator&) = delete;
  ElectronPlatformPasskeysAuthenticator& operator=(
      const ElectronPlatformPasskeysAuthenticator&) = delete;

  static bool IsAvailable();

  // device::FidoAuthenticator:
  void InitializeAuthenticator(base::OnceClosure callback) override;
  void MakeCredential(device::CtapMakeCredentialRequest request,
                      device::MakeCredentialOptions options,
                      MakeCredentialCallback callback) override;
  void GetAssertion(device::CtapGetAssertionRequest request,
                    device::CtapGetAssertionOptions options,
                    GetAssertionCallback callback) override;
  void GetPlatformCredentialInfoForRequest(
      const device::CtapGetAssertionRequest& request,
      const device::CtapGetAssertionOptions& options,
      GetPlatformCredentialInfoForRequestCallback callback) override;
  void Cancel() override;
  // Returns kICloudKeychain — the upstream Chromium enum value for
  // ASAuthorizationController-backed passkeys. Electron's public API uses the
  // name "platformPasskeys" because the system credential provider is not
  // necessarily iCloud Keychain (1Password, Bitwarden, etc. can fulfill these).
  device::AuthenticatorType GetType() const override;
  std::string GetId() const override;
  const device::AuthenticatorSupportedOptions& Options() const override;
  std::optional<device::FidoTransportProtocol> AuthenticatorTransport()
      const override;
  base::WeakPtr<FidoAuthenticator> GetWeakPtr() override;

 private:
  struct ObjCStorage;

  void OnMakeCredentialComplete(MakeCredentialCallback callback);
  void OnGetAssertionComplete(GetAssertionCallback callback);

  const content::GlobalRenderFrameHostToken render_frame_host_token_;
  std::unique_ptr<ObjCStorage> objc_storage_;
  base::WeakPtrFactory<ElectronPlatformPasskeysAuthenticator> weak_factory_{
      this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_PLATFORM_PASSKEYS_AUTHENTICATOR_H_
