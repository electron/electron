// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/webauthn/electron_authenticator_request_client_delegate.h"

#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "base/base64url.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/public/public_key_credential_descriptor.h"
#include "device/fido/public/public_key_credential_user_entity.h"
#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"

#if DCHECK_IS_ON()
#include "base/memory/scoped_refptr.h"
#include "device/fido/fido_device_discovery.h"
#include "device/fido/public/fido_constants.h"
#include "device/fido/virtual_ctap2_device.h"
#include "device/fido/virtual_fido_device.h"
#include "device/fido/virtual_fido_device_authenticator.h"
#endif

namespace electron {

namespace {

// WebAuthn's PublicKeyCredential.id is canonically URL-safe base64 with no
// padding, so encode credential IDs and user handles the same way to keep the
// event payload string-comparable to values returned by navigator.credentials.
std::string Base64UrlEncodeNoPad(base::span<const uint8_t> input) {
  std::string out;
  base::Base64UrlEncode(input, base::Base64UrlEncodePolicy::OMIT_PADDING, &out);
  return out;
}

std::string CredentialIdFor(
    const device::AuthenticatorGetAssertionResponse& response) {
  if (response.credential) {
    return Base64UrlEncodeNoPad(response.credential->id);
  }
  return {};
}

#if DCHECK_IS_ON()
bool g_simulate_uv_locked_pin_security_key = false;

// Yields one virtual CTAP 2.1 security key in the state reported in
// https://github.com/electron/electron/issues/54317: built-in user verification
// is configured but has no retries left and a PIN is set, so Chromium reads
// the retry counts and then falls back to collecting the PIN.
class UvLockedPinSecurityKeyDiscovery final
    : public device::FidoDeviceDiscovery {
 public:
  UvLockedPinSecurityKeyDiscovery()
      : device::FidoDeviceDiscovery(device::FidoTransportProtocol::kInternal) {}

 private:
  void StartInternal() override {
    auto state = base::MakeRefCounted<device::VirtualFidoDevice::State>();
    state->transport = device::FidoTransportProtocol::kInternal;
    state->fingerprints_enrolled = true;
    state->uv_retries = 0;
    state->pin = "123456";

    device::VirtualCtap2Device::Config config;
    config.ctap2_versions = {std::begin(device::kCtap2Versions2_1),
                             std::end(device::kCtap2Versions2_1)};
    config.is_platform_authenticator = true;
    config.internal_uv_support = true;
    config.pin_support = true;
    config.pin_uv_auth_token_support = true;
    config.always_uv = true;

    // GetAssertion probes platform authenticators for matching credentials
    // before dispatch, which only the virtual authenticator wrapper answers.
    AddAuthenticator(std::make_unique<device::VirtualFidoDeviceAuthenticator>(
        std::make_unique<device::VirtualCtap2Device>(std::move(state),
                                                     config)));
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&UvLockedPinSecurityKeyDiscovery::NotifyDiscoveryStarted,
                       weak_factory_.GetWeakPtr(), /*success=*/true));
  }

  base::WeakPtrFactory<UvLockedPinSecurityKeyDiscovery> weak_factory_{this};
};
#endif

}  // namespace

#if DCHECK_IS_ON()
// static
void ElectronAuthenticatorRequestClientDelegate::
    SetSimulateUvLockedPinSecurityKeyForTesting(bool enabled) {
  g_simulate_uv_locked_pin_security_key = enabled;
}
#endif

ElectronAuthenticatorRequestClientDelegate::
    ElectronAuthenticatorRequestClientDelegate(
        content::RenderFrameHost* render_frame_host)
    : render_frame_host_id_(render_frame_host->GetGlobalId()) {}

ElectronAuthenticatorRequestClientDelegate::
    ~ElectronAuthenticatorRequestClientDelegate() = default;

void ElectronAuthenticatorRequestClientDelegate::SetRelyingPartyId(
    const std::string& rp_id) {
  relying_party_id_ = rp_id;
}

void ElectronAuthenticatorRequestClientDelegate::StartObserving(
    device::FidoRequestHandlerBase* request_handler) {
  request_handler_observation_.Observe(request_handler);
}

void ElectronAuthenticatorRequestClientDelegate::StopObserving(
    device::FidoRequestHandlerBase* request_handler) {
  request_handler_observation_.Reset();
}

void ElectronAuthenticatorRequestClientDelegate::RegisterActionCallbacks(
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
        request_ble_permission_callback) {
  cancel_callback_ = std::move(cancel_callback);
}

void ElectronAuthenticatorRequestClientDelegate::SelectAccount(
    std::vector<device::AuthenticatorGetAssertionResponse> responses,
    base::OnceCallback<void(device::AuthenticatorGetAssertionResponse)>
        callback) {
  DCHECK(!responses.empty());

  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_frame_host_id_);
  content::WebContents* web_contents =
      rfh ? content::WebContents::FromRenderFrameHost(rfh) : nullptr;
  gin::WeakCell<api::Session>* session =
      web_contents
          ? api::Session::FromBrowserContext(web_contents->GetBrowserContext())
          : nullptr;

  pending_responses_ = std::move(responses);
  select_account_callback_ = std::move(callback);

  if (!session || !session->Get()) {
    CancelPendingAccountSelection();
    return;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  v8::Local<v8::Array> accounts =
      v8::Array::New(isolate, static_cast<int>(pending_responses_.size()));
  for (size_t i = 0; i < pending_responses_.size(); ++i) {
    const auto& response = pending_responses_[i];
    gin::DataObjectBuilder account(isolate);
    account.Set("credentialId", CredentialIdFor(response));
    if (response.user_entity) {
      account.Set("userHandle", Base64UrlEncodeNoPad(response.user_entity->id));
      if (response.user_entity->name) {
        account.Set("name", *response.user_entity->name);
      }
      if (response.user_entity->display_name) {
        account.Set("displayName", *response.user_entity->display_name);
      }
    }
    accounts
        ->CreateDataProperty(isolate->GetCurrentContext(),
                             static_cast<uint32_t>(i), account.Build())
        .Check();
  }

  v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                      .Set("relyingPartyId", relying_party_id_)
                                      .Set("accounts", accounts)
                                      .Set("frame", rfh)
                                      .Build();

  v8::Local<v8::Object> session_wrapper;
  if (!session->Get()->GetWrapper(isolate).ToLocal(&session_wrapper)) {
    CancelPendingAccountSelection();
    return;
  }

  v8::Local<v8::Object> event_object = gin_helper::internal::Event::New(isolate)
                                           ->GetWrapper(isolate)
                                           .ToLocalChecked();

  // A listener that runs the callback synchronously completes the request,
  // which may destroy |this| before EmitEvent returns.
  base::WeakPtr<ElectronAuthenticatorRequestClientDelegate> weak_this =
      weak_factory_.GetWeakPtr();
  v8::Local<v8::Value> emit_result = gin_helper::EmitEvent(
      isolate, session_wrapper, "select-webauthn-account", event_object,
      details,
      base::BindRepeating(
          &ElectronAuthenticatorRequestClientDelegate::OnAccountSelected,
          weak_factory_.GetWeakPtr()));
  if (!weak_this) {
    return;
  }

  // EventEmitter.prototype.emit() returns true iff there was at least one
  // listener. With no listener there is no way for the app to choose an
  // account, so cancel rather than silently picking one.
  bool had_listener = false;
  if (!gin::ConvertFromV8(isolate, emit_result, &had_listener) ||
      !had_listener) {
    CancelPendingAccountSelection();
  }
}

void ElectronAuthenticatorRequestClientDelegate::
    CancelPendingAccountSelection() {
  pending_responses_.clear();
  select_account_callback_.Reset();
  CancelRequest();
}

void ElectronAuthenticatorRequestClientDelegate::CancelRequest() {
  if (cancel_callback_) {
    std::move(cancel_callback_).Run();
  }
}

void ElectronAuthenticatorRequestClientDelegate::OnAccountSelected(
    gin::Arguments* args) {
  if (!select_account_callback_) {
    return;
  }

  std::string credential_id;
  if (!args->GetNext(&credential_id) || credential_id.empty()) {
    CancelPendingAccountSelection();
    return;
  }

  for (auto& response : pending_responses_) {
    if (CredentialIdFor(response) == credential_id) {
      auto selected = std::move(response);
      pending_responses_.clear();
      std::move(select_account_callback_).Run(std::move(selected));
      return;
    }
  }

  // Unknown credentialId: cancel the pending request rather than leaving it
  // hanging. Matches the no-args branch above so the listener has a single,
  // consistent failure mode whether it cancels deliberately or by mistake.
  CancelPendingAccountSelection();
}

void ElectronAuthenticatorRequestClientDelegate::CollectPIN(
    CollectPINOptions options,
    base::OnceCallback<void(std::u16string)> provide_pin_cb) {
  // SupportsPIN() is false, so Chromium never plans to use a PIN, but it still
  // ends up here when a security key's built-in user verification is locked or
  // gets blocked mid-request and the key has a PIN to fall back to. There is no
  // PIN prompt, so fail the request rather than hit the default NOTREACHED().
  if (auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_)) {
    rfh->AddMessageToConsole(
        blink::mojom::ConsoleMessageLevel::kWarning,
        "The security key needs its PIN to continue, but Electron does not "
        "support WebAuthn PIN entry "
        "(https://github.com/electron/electron/issues/24573). The request "
        "was cancelled.");
  }
  // This runs inside the FIDO device's response handling, which cancelling
  // destroys, so cancel from a fresh task.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&ElectronAuthenticatorRequestClientDelegate::CancelRequest,
                     weak_factory_.GetWeakPtr()));
}

std::vector<std::unique_ptr<device::FidoDiscoveryBase>>
ElectronAuthenticatorRequestClientDelegate::CreatePlatformDiscoveries() {
  std::vector<std::unique_ptr<device::FidoDiscoveryBase>> discoveries;
#if DCHECK_IS_ON()
  if (g_simulate_uv_locked_pin_security_key) {
    discoveries.push_back(std::make_unique<UvLockedPinSecurityKeyDiscovery>());
  }
#endif
  return discoveries;
}

}  // namespace electron
