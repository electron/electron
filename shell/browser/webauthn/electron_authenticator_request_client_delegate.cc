// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/webauthn/electron_authenticator_request_client_delegate.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/base64url.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/values.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/fido_user_verification_requirement.h"
#include "device/fido/public/fido_constants.h"
#include "device/fido/public/fido_transport_protocol.h"
#include "device/fido/public/public_key_credential_descriptor.h"
#include "device/fido/public/public_key_credential_user_entity.h"
#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter_caller.h"

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

api::Session* ResolveSession(
    const base::WeakPtr<content::WebContents>& web_contents_weak) {
  content::WebContents* web_contents = web_contents_weak.get();
  if (!web_contents) {
    return nullptr;
  }
  gin::WeakCell<api::Session>* session =
      api::Session::FromBrowserContext(web_contents->GetBrowserContext());
  return session ? session->Get() : nullptr;
}

std::string_view RequestTypeToString(device::FidoRequestType request_type) {
  switch (request_type) {
    case device::FidoRequestType::kMakeCredential:
      return "create";
    case device::FidoRequestType::kGetAssertion:
      return "get";
  }
}

std::string_view FailureReasonToString(
    content::AuthenticatorRequestClientDelegate::InterestingFailureReason
        reason) {
  using InterestingFailureReason =
      content::AuthenticatorRequestClientDelegate::InterestingFailureReason;
  switch (reason) {
    case InterestingFailureReason::kTimeout:
      return "timeout";
    case InterestingFailureReason::kKeyNotRegistered:
      return "key-not-registered";
    case InterestingFailureReason::kKeyAlreadyRegistered:
      return "key-already-registered";
    case InterestingFailureReason::kSoftPINBlock:
      return "pin-soft-blocked";
    case InterestingFailureReason::kHardPINBlock:
      return "pin-hard-blocked";
    case InterestingFailureReason::kAuthenticatorRemovedDuringPINEntry:
      return "authenticator-removed-during-pin-entry";
    case InterestingFailureReason::kAuthenticatorMissingResidentKeys:
      return "missing-resident-keys";
    case InterestingFailureReason::kAuthenticatorMissingUserVerification:
      return "missing-user-verification";
    case InterestingFailureReason::kAuthenticatorMissingLargeBlob:
      return "missing-large-blob";
    case InterestingFailureReason::kNoCommonAlgorithms:
      return "no-common-algorithms";
    case InterestingFailureReason::kStorageFull:
      return "storage-full";
    case InterestingFailureReason::kUserConsentDenied:
      return "user-consent-denied";
    case InterestingFailureReason::kWinUserCancelled:
      return "win-user-cancelled";
    case InterestingFailureReason::kHybridTransportError:
      return "hybrid-transport-error";
    case InterestingFailureReason::kNoPasskeys:
      return "no-passkeys";
    case InterestingFailureReason::kEnclaveError:
      return "enclave-error";
    case InterestingFailureReason::kEnclaveCancel:
      return "enclave-cancel";
  }
}

}  // namespace

ElectronAuthenticatorRequestClientDelegate::
    ElectronAuthenticatorRequestClientDelegate(
        content::RenderFrameHost* render_frame_host)
    : render_frame_host_id_(render_frame_host->GetGlobalId()),
      web_contents_(content::WebContents::FromRenderFrameHost(render_frame_host)
                        ->GetWeakPtr()) {}

ElectronAuthenticatorRequestClientDelegate::
    ~ElectronAuthenticatorRequestClientDelegate() {
  // A started request must always announce completion so app-drawn UI can be
  // dismissed, even when it ends without a specific outcome (abort, teardown).
  EmitRequestCompleted(false, {});
}

void ElectronAuthenticatorRequestClientDelegate::SetRelyingPartyId(
    const std::string& rp_id) {
  relying_party_id_ = rp_id;
}

void ElectronAuthenticatorRequestClientDelegate::SetUIPresentation(
    UIPresentation ui_presentation) {
  ui_presentation_ = ui_presentation;
}

bool ElectronAuthenticatorRequestClientDelegate::DoesBlockRequestOnFailure(
    InterestingFailureReason reason) {
  EmitRequestCompleted(false, FailureReasonToString(reason));
  // Never hold the request open; the page receives the error immediately and
  // the app surfaces it via the event above.
  return false;
}

void ElectronAuthenticatorRequestClientDelegate::OnTransactionSuccessful(
    RequestSource request_source,
    device::FidoRequestType request_type,
    device::AuthenticatorType authenticator_type) {
  EmitRequestCompleted(true, {});
}

void ElectronAuthenticatorRequestClientDelegate::StartObserving(
    device::FidoRequestHandlerBase* request_handler) {
  // Reset defensively: a start-over would arrive with the observation still
  // bound to the previous (destroyed) request handler.
  request_handler_observation_.Reset();
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

void ElectronAuthenticatorRequestClientDelegate::
    OnTransportAvailabilityEnumerated(
        device::FidoRequestHandlerBase::TransportAvailabilityInfo data) {
  if (!ShouldEmitCeremonyEvents()) {
    return;
  }

  base::ListValue transports;
  for (const auto transport : data.available_transports) {
    transports.Append(device::ToString(transport));
  }

  base::DictValue details;
  details.Set("relyingPartyId", relying_party_id_);
  details.Set("requestType", RequestTypeToString(data.request_type));
  details.Set("transports", std::move(transports));
  details.Set("userVerification",
              device::ToString(data.user_verification_requirement));

  completed_event_pending_ = true;
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ElectronAuthenticatorRequestClientDelegate::EmitCeremonyEvent,
          web_contents_, render_frame_host_id_,
          std::string("webauthn-request-started"), std::move(details)));
}

void ElectronAuthenticatorRequestClientDelegate::SelectAccount(
    std::vector<device::AuthenticatorGetAssertionResponse> responses,
    base::OnceCallback<void(device::AuthenticatorGetAssertionResponse)>
        callback) {
  DCHECK(!responses.empty());

  pending_responses_ = std::move(responses);
  select_account_callback_ = std::move(callback);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  content::RenderFrameHost* rfh = nullptr;
  v8::Local<v8::Object> session_wrapper;
  if (!GetSessionWrapper(isolate, &rfh).ToLocal(&session_wrapper)) {
    CancelPendingAccountSelection();
    return;
  }

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

  v8::Local<v8::Object> event_object = gin_helper::internal::Event::New(isolate)
                                           ->GetWrapper(isolate)
                                           .ToLocalChecked();

  v8::Local<v8::Value> emit_result = gin_helper::EmitEvent(
      isolate, session_wrapper, "select-webauthn-account", event_object,
      details,
      base::BindRepeating(
          &ElectronAuthenticatorRequestClientDelegate::OnAccountSelected,
          weak_factory_.GetWeakPtr()));

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

bool ElectronAuthenticatorRequestClientDelegate::ShouldEmitCeremonyEvents()
    const {
  // Only tab-modal presentations expect app-drawn ceremony UI; autofill/
  // ambient/upgrade requests run in the background without dialogs.
  return ui_presentation_ == UIPresentation::kModal ||
         ui_presentation_ == UIPresentation::kModalImmediate;
}

v8::MaybeLocal<v8::Object>
ElectronAuthenticatorRequestClientDelegate::GetSessionWrapper(
    v8::Isolate* isolate,
    content::RenderFrameHost** out_rfh) {
  // The initiating frame can die mid-ceremony while the request lives on, so
  // it is informational; the session comes from the captured WebContents.
  *out_rfh = content::RenderFrameHost::FromID(render_frame_host_id_);

  api::Session* session = ResolveSession(web_contents_);
  if (!session) {
    return {};
  }
  return session->GetWrapper(isolate);
}

// static
void ElectronAuthenticatorRequestClientDelegate::EmitCeremonyEvent(
    base::WeakPtr<content::WebContents> web_contents,
    content::GlobalRenderFrameHostId rfh_id,
    std::string name,
    base::DictValue details) {
  if (Browser::Get()->is_shutting_down()) {
    return;
  }
  api::Session* session = ResolveSession(web_contents);
  if (!session) {
    return;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  v8::Local<v8::Object> details_object =
      gin::ConvertToV8(isolate, details).As<v8::Object>();
  details_object
      ->CreateDataProperty(
          isolate->GetCurrentContext(), gin::StringToSymbol(isolate, "frame"),
          gin::ConvertToV8(isolate, content::RenderFrameHost::FromID(rfh_id)))
      .Check();

  session->Emit(name, details_object);
}

void ElectronAuthenticatorRequestClientDelegate::EmitRequestCompleted(
    bool success,
    std::string_view reason) {
  if (!completed_event_pending_) {
    return;
  }
  completed_event_pending_ = false;

  base::DictValue details;
  details.Set("relyingPartyId", relying_party_id_);
  details.Set("success", success);
  if (!reason.empty()) {
    details.Set("reason", reason);
  }

  // Posted: this runs from content completion paths and the destructor,
  // where inline app JS could re-enter half-destroyed objects.
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ElectronAuthenticatorRequestClientDelegate::EmitCeremonyEvent,
          web_contents_, render_frame_host_id_,
          std::string("webauthn-request-completed"), std::move(details)));
}

}  // namespace electron
