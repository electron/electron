// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/webauthn/electron_authenticator_request_client_delegate.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/base64url.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/fido_authenticator.h"
#include "device/fido/public/fido_types.h"
#include "device/fido/public/public_key_credential_descriptor.h"
#include "device/fido/public/public_key_credential_user_entity.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/data_object_builder.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter_caller.h"

#if BUILDFLAG(IS_MAC)
#include "shell/browser/webauthn/electron_authenticator_request_delegate.h"
#include "shell/browser/webauthn/electron_platform_passkeys_discovery.h"
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

}  // namespace

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
  request_callback_ = std::move(request_callback);
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

bool ElectronAuthenticatorRequestClientDelegate::
    EmbedderControlsAuthenticatorDispatch(
        const device::FidoAuthenticator& authenticator) {
#if BUILDFLAG(IS_MAC)
  if (authenticator.AuthenticatorTransport() !=
      device::FidoTransportProtocol::kInternal) {
    return false;
  }

  // Only intercept dispatch when both Touch ID and platform passkeys are
  // configured — that's the only scenario where dual prompts can appear.
  // When only one is configured, Chromium's auto-dispatch is correct.
  if (!ElectronWebAuthenticationDelegate::IsPlatformPasskeysEnabled() ||
      !ElectronWebAuthenticationDelegate::IsTouchIdConfigured()) {
    return false;
  }

  auto type = authenticator.GetType();
  if (type == device::AuthenticatorType::kTouchID ||
      type == device::AuthenticatorType::kICloudKeychain) {
    controls_dispatch_ = true;
    return true;
  }
#endif
  return false;
}

void ElectronAuthenticatorRequestClientDelegate::FidoAuthenticatorAdded(
    const device::FidoAuthenticator& authenticator) {
  if (!controls_dispatch_)
    return;

  std::string display_name;
  switch (authenticator.GetType()) {
    case device::AuthenticatorType::kTouchID:
      display_name = "touchID";
      break;
    case device::AuthenticatorType::kICloudKeychain:
      display_name = "platformPasskeys";
      break;
    default:
      return;
  }

  pending_authenticators_.push_back(
      {authenticator.GetId(), std::move(display_name)});
}

void ElectronAuthenticatorRequestClientDelegate::
    OnTransportAvailabilityEnumerated(
        device::FidoRequestHandlerBase::TransportAvailabilityInfo data) {
  if (!controls_dispatch_ || pending_authenticators_.empty())
    return;
  MaybeEmitSelectAuthenticatorEvent();
}

void ElectronAuthenticatorRequestClientDelegate::
    MaybeEmitSelectAuthenticatorEvent() {
  if (pending_authenticators_.size() == 1) {
    request_callback_.Run(pending_authenticators_[0].id);
    pending_authenticators_.clear();
    return;
  }

  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_frame_host_id_);
  content::WebContents* web_contents =
      rfh ? content::WebContents::FromRenderFrameHost(rfh) : nullptr;
  gin::WeakCell<api::Session>* session =
      web_contents
          ? api::Session::FromBrowserContext(web_contents->GetBrowserContext())
          : nullptr;

  if (!session || !session->Get()) {
    DispatchDefaultAuthenticator();
    return;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  std::vector<std::string> authenticators;
  authenticators.reserve(pending_authenticators_.size());
  for (const auto& authenticator : pending_authenticators_) {
    authenticators.push_back(authenticator.display_name);
  }

  v8::Local<v8::Object> session_wrapper;
  if (!session->Get()->GetWrapper(isolate).ToLocal(&session_wrapper)) {
    DispatchDefaultAuthenticator();
    return;
  }

  gin_helper::internal::Event* event =
      gin_helper::internal::Event::New(isolate);
  v8::Local<v8::Object> event_object =
      event->GetWrapper(isolate).ToLocalChecked();

  gin_helper::Dictionary dict(isolate, event_object);
  dict.Set("relyingPartyId", relying_party_id_);
  dict.Set("authenticators", authenticators);
  dict.SetGetter("frame", rfh);

  v8::Local<v8::Value> emit_result = gin_helper::EmitEvent(
      isolate, session_wrapper, "select-webauthn-authenticator", event_object,
      base::BindRepeating(
          &ElectronAuthenticatorRequestClientDelegate::OnAuthenticatorSelected,
          weak_factory_.GetWeakPtr()));

  bool had_listener = false;
  if (!gin::ConvertFromV8(isolate, emit_result, &had_listener) ||
      !had_listener) {
    DispatchDefaultAuthenticator();
  }
}

void ElectronAuthenticatorRequestClientDelegate::
    DispatchDefaultAuthenticator() {
  auto auth = std::ranges::find(pending_authenticators_, "platformPasskeys",
                                &PendingAuthenticator::display_name);
  request_callback_.Run(auth != pending_authenticators_.end()
                            ? auth->id
                            : pending_authenticators_.front().id);
  pending_authenticators_.clear();
}

void ElectronAuthenticatorRequestClientDelegate::OnAuthenticatorSelected(
    gin::Arguments* args) {
  // The emit callback is a repeating callback, so guard against an app invoking
  // it more than once: after the first call clears pending_authenticators_, a
  // second call would otherwise fall through to the unknown-name branch and
  // cancel a request we already dispatched.
  if (pending_authenticators_.empty()) {
    return;
  }

  std::string selected_name;
  if (!args->GetNext(&selected_name) || selected_name.empty()) {
    if (cancel_callback_) {
      std::move(cancel_callback_).Run();
    }
    pending_authenticators_.clear();
    return;
  }

  for (const auto& auth : pending_authenticators_) {
    if (auth.display_name == selected_name) {
      request_callback_.Run(auth.id);
      pending_authenticators_.clear();
      return;
    }
  }

  // Unknown name — cancel.
  if (cancel_callback_) {
    std::move(cancel_callback_).Run();
  }
  pending_authenticators_.clear();
}

#if BUILDFLAG(IS_MAC)
std::vector<std::unique_ptr<device::FidoDiscoveryBase>>
ElectronAuthenticatorRequestClientDelegate::CreatePlatformDiscoveries() {
  std::vector<std::unique_ptr<device::FidoDiscoveryBase>> discoveries;
  if (ElectronWebAuthenticationDelegate::IsPlatformPasskeysEnabled()) {
    auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
    if (rfh) {
      discoveries.push_back(
          std::make_unique<ElectronPlatformPasskeysDiscovery>(rfh));
    }
  }
  return discoveries;
}
#endif

}  // namespace electron
