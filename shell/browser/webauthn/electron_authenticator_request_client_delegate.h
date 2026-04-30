// Copyright (c) 2026 Microsoft, GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/authenticator_request_client_delegate.h"
#include "content/public/browser/global_routing_id.h"

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

  // content::AuthenticatorRequestClientDelegate:
  void SetRelyingPartyId(const std::string& rp_id) override;
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

 private:
  void OnAccountSelected(gin::Arguments* args);
  void CancelPendingAccountSelection();

  const content::GlobalRenderFrameHostId render_frame_host_id_;
  std::string relying_party_id_;
  base::OnceClosure cancel_callback_;

  std::vector<device::AuthenticatorGetAssertionResponse> pending_responses_;
  base::OnceCallback<void(device::AuthenticatorGetAssertionResponse)>
      select_account_callback_;

  base::WeakPtrFactory<ElectronAuthenticatorRequestClientDelegate>
      weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_
