// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/dcheck_is_on.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "build/build_config.h"
#include "content/public/browser/authenticator_request_client_delegate.h"
#include "content/public/browser/global_routing_id.h"
#include "device/fido/cable/v2_constants.h"
#include "device/fido/fido_discovery_base.h"
#include "device/fido/fido_request_handler_base.h"
#include "device/fido/public/fido_constants.h"

namespace device {
class BluetoothAdapter;
}  // namespace device

namespace content {
class RenderFrameHost;
}

namespace gin {
class Arguments;
template <typename T>
class WeakCell;
}  // namespace gin

namespace electron {

class ElectronBrowserContext;

namespace api {
class Session;
}

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
  void SetUIPresentation(UIPresentation ui_presentation) override;
  void ConfigureDiscoveries(
      const url::Origin& origin,
      const std::string& rp_id,
      RequestSource request_source,
      device::FidoRequestType request_type,
      std::optional<device::ResidentKeyRequirement> resident_key_requirement,
      device::UserVerificationRequirement user_verification_requirement,
      bool cmtg_key_requested,
      std::optional<std::string_view> user_name,
      bool is_enclave_authenticator_available,
      device::FidoDiscoveryFactory* fido_discovery_factory) override;
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
  bool EmbedderControlsAuthenticatorDispatch(
      const device::FidoAuthenticator& authenticator) override;
  void FidoAuthenticatorAdded(
      const device::FidoAuthenticator& authenticator) override;
  void OnTransportAvailabilityEnumerated(
      device::FidoRequestHandlerBase::TransportAvailabilityInfo data) override;
  bool SupportsPIN() const override;
  void CollectPIN(
      CollectPINOptions options,
      base::OnceCallback<void(std::u16string)> provide_pin_cb) override;
  void StartBioEnrollment(base::OnceClosure next_callback) override;
  std::vector<std::unique_ptr<device::FidoDiscoveryBase>>
  CreatePlatformDiscoveries() override;

 private:
  struct PendingAuthenticator {
    std::string id;
    std::string display_name;
  };

  gin::WeakCell<api::Session>* GetSession() const;
  void OnAccountSelected(gin::Arguments* args);
  void CancelPendingAccountSelection();
  void CancelRequest();
  // Emits 'webauthn-hybrid-request' on the session. Returns true iff at least
  // one listener received the event.
  bool EmitHybridRequestEvent(device::FidoRequestType request_type,
                              const std::string& qr_code);
  void OnBleStatus(device::FidoRequestHandlerBase::BleStatus status);
  void MaybeEmitSelectAuthenticatorEvent();
  void DispatchDefaultAuthenticator();
  void OnAuthenticatorSelected(gin::Arguments* args);
  void StartOver();
  void RestartWithBluetoothAdapter(
      scoped_refptr<device::BluetoothAdapter> adapter);
  void EmitCollectPinEvent(const CollectPINOptions& options);
  void OnPinProvided(uint64_t prompt_id, gin::Arguments* args);
  void CancelPendingPinCollection();

  const content::GlobalRenderFrameHostId render_frame_host_id_;
  std::string relying_party_id_;
  UIPresentation ui_presentation_ = UIPresentation::kModal;
  base::OnceClosure cancel_callback_;
  device::FidoRequestHandlerBase::RequestCallback request_callback_;
  base::RepeatingCallback<void(
      device::FidoRequestHandlerBase::BlePermissionCallback)>
      request_ble_permission_callback_;

  // Set once an app listener accepted the 'webauthn-hybrid-request' event
  // and hybrid (caBLE v2) discovery was configured for this request. The
  // browser context outlives the request; the frame may not, so the
  // completion event is delivered through this handle.
  base::WeakPtr<ElectronBrowserContext> hybrid_browser_context_;
  // The key behind the QR code the app was given; reused if the request
  // restarts so the code stays valid.
  std::optional<std::array<uint8_t, device::cablev2::kQRKeySize>>
      hybrid_qr_generator_key_;
  base::RepeatingClosure start_over_callback_;
  // Held across a restart; see RestartWithBluetoothAdapter().
  scoped_refptr<device::BluetoothAdapter> bluetooth_adapter_;

  base::ScopedObservation<device::FidoRequestHandlerBase,
                          device::FidoRequestHandlerBase::Observer>
      request_handler_observation_{this};

  std::vector<device::AuthenticatorGetAssertionResponse> pending_responses_;
  base::OnceCallback<void(device::AuthenticatorGetAssertionResponse)>
      select_account_callback_;

  std::vector<PendingAuthenticator> pending_authenticators_;
  bool controls_dispatch_ = false;

  base::OnceCallback<void(std::u16string)> provide_pin_callback_;
  // Identifies the current 'collect-webauthn-pin' emission.
  uint64_t pin_prompt_id_ = 0;

  base::WeakPtrFactory<ElectronAuthenticatorRequestClientDelegate>
      weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_
