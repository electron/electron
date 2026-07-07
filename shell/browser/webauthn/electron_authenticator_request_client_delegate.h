// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_AUTHENTICATOR_REQUEST_CLIENT_DELEGATE_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "content/public/browser/authenticator_request_client_delegate.h"
#include "content/public/browser/global_routing_id.h"
#include "v8/include/v8-forward.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

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
  void SetUIPresentation(UIPresentation ui_presentation) override;
  bool DoesBlockRequestOnFailure(InterestingFailureReason reason) override;
  void OnTransactionSuccessful(
      RequestSource request_source,
      device::FidoRequestType request_type,
      device::AuthenticatorType authenticator_type) override;
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

  // device::FidoRequestHandlerBase::Observer:
  void OnTransportAvailabilityEnumerated(
      device::FidoRequestHandlerBase::TransportAvailabilityInfo data) override;

 private:
  void OnAccountSelected(gin::Arguments* args);
  void CancelPendingAccountSelection();

  // Whether ceremony lifecycle events should be emitted for this request;
  // non-modal presentations must not trigger app-drawn modal UI.
  bool ShouldEmitCeremonyEvents() const;

  // Resolves the api::Session wrapper for this request's WebContents, or an
  // empty handle if it is gone. |out_rfh| may be set to null mid-ceremony.
  v8::MaybeLocal<v8::Object> GetSessionWrapper(
      v8::Isolate* isolate,
      content::RenderFrameHost** out_rfh);

  // Emits a fire-and-forget ceremony event on the session from a fresh task.
  // Static and bound to plain data only: it is posted from FIDO/content
  // callstacks (and the destructor), which app JS must not re-enter.
  static void EmitCeremonyEvent(
      base::WeakPtr<content::WebContents> web_contents,
      content::GlobalRenderFrameHostId rfh_id,
      std::string name,
      base::DictValue details);

  void EmitRequestCompleted(bool success, std::string_view reason);

  const content::GlobalRenderFrameHostId render_frame_host_id_;
  base::WeakPtr<content::WebContents> web_contents_;
  std::string relying_party_id_;
  base::OnceClosure cancel_callback_;

  UIPresentation ui_presentation_ = UIPresentation::kModal;

  // True between the started emit and its matching completed emit; enforces
  // the exactly-one-completed-per-started pairing.
  bool completed_event_pending_ = false;

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
