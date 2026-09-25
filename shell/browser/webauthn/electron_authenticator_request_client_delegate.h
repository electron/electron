// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/dcheck_is_on.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "content/public/browser/authenticator_request_client_delegate.h"
#include "content/public/browser/global_routing_id.h"
#include "device/fido/fido_discovery_base.h"

namespace content {
class RenderFrameHost;
}

namespace gin {
class Arguments;
}

namespace electron {

class ElectronAuthenticatorRequestClientDelegate
    : public content::AuthenticatorRequestClientDelegate {
 public:
  explicit ElectronAuthenticatorRequestClientDelegate(
      content::RenderFrameHost* render_frame_host);
  ~ElectronAuthenticatorRequestClientDelegate() override;

  // disable copy
  ElectronAuthenticatorRequestClientDelegate(
      const ElectronAuthenticatorRequestClientDelegate&) = delete;
  ElectronAuthenticatorRequestClientDelegate& operator=(
      const ElectronAuthenticatorRequestClientDelegate&) = delete;

#if DCHECK_IS_ON()
  // Makes every request see one virtual CTAP 2.1 security key whose built-in
  // user verification is locked and which has a PIN set, so that Chromium
  // falls back to asking for the PIN. DevTools' virtual authenticators cannot
  // be given a PIN; specs reach this through the testing binding instead.
  static void SetSimulateUvLockedPinSecurityKeyForTesting(bool enabled);
#endif

  // content::AuthenticatorRequestClientDelegate:
  void SetRelyingPartyId(const std::string& rp_id) override;
  void StartObserving(device::FidoRequestHandlerBase* request_handler) override;
  void StopObserving(device::FidoRequestHandlerBase* request_handler) override;
  void RegisterActionCallbacks(
      base::OnceClosure cancel_callback,
      base::OnceClosure immediate_not_found_callback,
      base::RepeatingClosure start_over_callback,
      AccountPreselectedCallback account_preselected_callback,
      PasswordSelectedCallback password_selected_callback,
      device::FidoRequestHandlerBase::RequestCallback request_callback,
      base::OnceClosure cancel_ui_timeout_callback,
      base::RepeatingClosure bluetooth_adapter_power_on_callback,
      base::RepeatingCallback<
          void(device::FidoRequestHandlerBase::BlePermissionCallback)>
          request_ble_permission_callback) override;
  void SelectAccount(
      std::vector<device::AuthenticatorGetAssertionResponse> responses,
      base::OnceCallback<void(device::AuthenticatorGetAssertionResponse)>
          callback) override;
  void CollectPIN(
      CollectPINOptions options,
      base::OnceCallback<void(std::u16string)> provide_pin_cb) override;
  std::vector<std::unique_ptr<device::FidoDiscoveryBase>>
  CreatePlatformDiscoveries() override;

 private:
  void OnAccountSelected(gin::Arguments* args);
  void CancelPendingAccountSelection();
  void CancelRequest();

  const content::GlobalRenderFrameHostId render_frame_host_id_;
  std::string relying_party_id_;
  base::OnceClosure cancel_callback_;

  base::ScopedObservation<device::FidoRequestHandlerBase,
                          device::FidoRequestHandlerBase::Observer>
      request_handler_observation_{this};

  std::vector<device::AuthenticatorGetAssertionResponse> pending_responses_;
  base::OnceCallback<void(device::AuthenticatorGetAssertionResponse)>
      select_account_callback_;

  base::WeakPtrFactory<ElectronAuthenticatorRequestClientDelegate>
      weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_
