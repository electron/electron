// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/webauthn/electron_authenticator_request_client_delegate.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

#include "base/base64url.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "components/device_event_log/device_event_log.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "crypto/random.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/cable/v2_constants.h"
#include "device/fido/cable/v2_handshake.h"
#include "device/fido/fido_authenticator.h"
#include "device/fido/fido_discovery_factory.h"
#include "device/fido/public/fido_types.h"
#include "device/fido/public/public_key_credential_descriptor.h"
#include "device/fido/public/public_key_credential_user_entity.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/data_object_builder.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "url/origin.h"

#if BUILDFLAG(IS_MAC)
#include "shell/browser/webauthn/electron_authenticator_request_delegate.h"
#include "shell/browser/webauthn/electron_platform_passkeys_discovery.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "device/fido/win/webauthn_api.h"
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

gin::WeakCell<api::Session>* SessionFor(
    content::GlobalRenderFrameHostId render_frame_host_id) {
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_frame_host_id);
  content::WebContents* web_contents =
      rfh ? content::WebContents::FromRenderFrameHost(rfh) : nullptr;
  return web_contents ? api::Session::FromBrowserContext(
                            web_contents->GetBrowserContext())
                      : nullptr;
}

// Hybrid transport tunnels to the phone over a WebSocket, which needs a
// network context. Use the one belonging to the requesting session so proxy
// and certificate settings match the page that made the request. The request
// handler that owns the discovery is torn down with the frame, before the
// browser context, so the context is live for every call made during a
// request.
network::mojom::NetworkContext* NetworkContextFor(
    base::WeakPtr<ElectronBrowserContext> browser_context) {
  if (!browser_context) {
    return nullptr;
  }
  return browser_context->GetDefaultStoragePartition()->GetNetworkContext();
}

// True when Chromium would actually create a hybrid discovery for this
// process. Mirrors the checks in FidoDiscoveryFactory::Create(kHybrid) so the
// app is not asked to show a QR code that can never be scanned.
bool HybridTransportAvailable() {
  if (!device::BluetoothAdapterFactory::Get()->IsLowEnergySupported()) {
    return false;
  }
#if BUILDFLAG(IS_WIN)
  // Windows 11 implements hybrid natively and shows its own QR code.
  device::WinWebAuthnApi* const webauthn_api =
      device::WinWebAuthnApi::GetDefault();
  if (webauthn_api && webauthn_api->SupportsHybrid()) {
    return false;
  }
#endif
  return true;
}

const char* RequestTypeName(device::FidoRequestType request_type) {
  switch (request_type) {
    case device::FidoRequestType::kMakeCredential:
      return "create";
    case device::FidoRequestType::kGetAssertion:
      return "get";
  }
  NOTREACHED();
}

// Completion of a WebAuthn request is signalled by //content destroying the
// delegate, so this runs from a task posted by the destructor rather than
// re-entering JavaScript from inside teardown. The frame is often already
// gone at that point (navigation is the usual reason a request ends early),
// so the session is resolved from the browser context instead.
void EmitHybridRequestCompleted(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    content::GlobalRenderFrameHostId render_frame_host_id,
    std::string relying_party_id) {
  gin::WeakCell<api::Session>* session =
      browser_context ? api::Session::FromBrowserContext(browser_context.get())
                      : nullptr;
  if (!session || !session->Get()) {
    return;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  v8::Local<v8::Object> session_wrapper;
  if (!session->Get()->GetWrapper(isolate).ToLocal(&session_wrapper)) {
    return;
  }

  v8::Local<v8::Object> details =
      gin::DataObjectBuilder(isolate)
          .Set("relyingPartyId", relying_party_id)
          .Set("frame", content::RenderFrameHost::FromID(render_frame_host_id))
          .Build();

  v8::Local<v8::Object> event_object = gin_helper::internal::Event::New(isolate)
                                           ->GetWrapper(isolate)
                                           .ToLocalChecked();
  gin_helper::EmitEvent(isolate, session_wrapper,
                        "webauthn-hybrid-request-completed", event_object,
                        details);
}

#if BUILDFLAG(IS_MAC)
// Mirrors Chromium's cross-origin (iframe) ceremony check.
bool IsSameOriginWithAncestors(content::RenderFrameHost* frame) {
  const url::Origin& origin = frame->GetLastCommittedOrigin();
  for (content::RenderFrameHost* parent = frame->GetParent(); parent;
       parent = parent->GetParent()) {
    if (!parent->GetLastCommittedOrigin().IsSameOriginWith(origin)) {
      return false;
    }
  }
  return true;
}
#endif

}  // namespace

ElectronAuthenticatorRequestClientDelegate::
    ElectronAuthenticatorRequestClientDelegate(
        content::RenderFrameHost* render_frame_host)
    : render_frame_host_id_(render_frame_host->GetGlobalId()) {}

ElectronAuthenticatorRequestClientDelegate::
    ~ElectronAuthenticatorRequestClientDelegate() {
  if (hybrid_browser_context_) {
    // During browser shutdown the task runner may refuse the task; the app is
    // going away with it, so the dropped event is harmless.
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&EmitHybridRequestCompleted, hybrid_browser_context_,
                       render_frame_host_id_, relying_party_id_));
  }
}

void ElectronAuthenticatorRequestClientDelegate::SetRelyingPartyId(
    const std::string& rp_id) {
  relying_party_id_ = rp_id;
}

void ElectronAuthenticatorRequestClientDelegate::SetUIPresentation(
    UIPresentation ui_presentation) {
  ui_presentation_ = ui_presentation;
}

void ElectronAuthenticatorRequestClientDelegate::ConfigureDiscoveries(
    const url::Origin& origin,
    const std::string& rp_id,
    RequestSource request_source,
    device::FidoRequestType request_type,
    std::optional<device::ResidentKeyRequirement> resident_key_requirement,
    device::UserVerificationRequirement user_verification_requirement,
    bool cmtg_key_requested,
    std::optional<std::string_view> user_name,
    bool is_enclave_authenticator_available,
    device::FidoDiscoveryFactory* fido_discovery_factory) {
  // A null factory means the request is for passwords only. Conditional
  // (autofill) and passkey-upgrade requests show no UI in Chromium either, so
  // a QR code must not appear for them.
  if (!fido_discovery_factory ||
      request_source != RequestSource::kWebAuthentication ||
      ui_presentation_ != UIPresentation::kModal ||
      !HybridTransportAvailable()) {
    return;
  }

  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_frame_host_id_);
  if (!rfh) {
    return;
  }
  base::WeakPtr<ElectronBrowserContext> browser_context =
      static_cast<ElectronBrowserContext*>(rfh->GetBrowserContext())
          ->GetWeakPtr();

  // Chromium adds kHybrid to every request's transport set but only creates
  // the hybrid discovery once a QR generator key is configured. Without one
  // the request has no hybrid authenticator and never completes on that
  // transport.
  std::array<uint8_t, device::cablev2::kQRKeySize> qr_generator_key;
  crypto::RandBytes(qr_generator_key);
  const std::string qr_code =
      device::cablev2::qr::Encode(qr_generator_key, request_type);

  // The app renders the QR code, so hybrid is only useful when a listener
  // exists. Without one, leave the transport unconfigured so the remaining
  // authenticators behave exactly as before.
  base::WeakPtr<ElectronAuthenticatorRequestClientDelegate> weak_this =
      weak_factory_.GetWeakPtr();
  if (!EmitHybridRequestEvent(request_type, qr_code) || !weak_this) {
    return;
  }

  hybrid_browser_context_ = browser_context;
  fido_discovery_factory->set_cable_data(request_type, qr_generator_key);
  fido_discovery_factory->set_network_context_factory(
      base::BindRepeating(&NetworkContextFor, browser_context));
}

bool ElectronAuthenticatorRequestClientDelegate::EmitHybridRequestEvent(
    device::FidoRequestType request_type,
    const std::string& qr_code) {
  gin::WeakCell<api::Session>* session = SessionFor(render_frame_host_id_);
  if (!session || !session->Get()) {
    return false;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  v8::Local<v8::Object> session_wrapper;
  if (!session->Get()->GetWrapper(isolate).ToLocal(&session_wrapper)) {
    return false;
  }

  v8::Local<v8::Object> details =
      gin::DataObjectBuilder(isolate)
          .Set("relyingPartyId", relying_party_id_)
          .Set("requestType", RequestTypeName(request_type))
          .Set("qrCode", qr_code)
          .Set("frame", content::RenderFrameHost::FromID(render_frame_host_id_))
          .Build();

  v8::Local<v8::Object> event_object = gin_helper::internal::Event::New(isolate)
                                           ->GetWrapper(isolate)
                                           .ToLocalChecked();

  v8::Local<v8::Value> emit_result =
      gin_helper::EmitEvent(isolate, session_wrapper, "webauthn-hybrid-request",
                            event_object, details);

  // EventEmitter.prototype.emit() returns true iff there was at least one
  // listener.
  bool had_listener = false;
  return gin::ConvertFromV8(isolate, emit_result, &had_listener) &&
         had_listener;
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
  bluetooth_adapter_power_on_callback_ =
      std::move(bluetooth_adapter_power_on_callback);
  request_ble_permission_callback_ = std::move(request_ble_permission_callback);
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
  // Hybrid discovery listens for the phone's BLE advertisement, which on
  // macOS needs the Bluetooth permission. Ask for it now rather than letting
  // the request sit until the OS prompt happens to appear.
  can_power_on_ble_adapter_ = data.can_power_on_ble_adapter;
  if (hybrid_browser_context_ && request_ble_permission_callback_ &&
      data.ble_status == device::FidoRequestHandlerBase::BleStatus::
                             kPendingPermissionRequest) {
    request_ble_permission_callback_.Run(
        base::BindOnce(&ElectronAuthenticatorRequestClientDelegate::OnBleStatus,
                       weak_factory_.GetWeakPtr()));
  }

  if (!controls_dispatch_ || pending_authenticators_.empty())
    return;
  MaybeEmitSelectAuthenticatorEvent();
}

void ElectronAuthenticatorRequestClientDelegate::OnBleStatus(
    device::FidoRequestHandlerBase::BleStatus status) {
  switch (status) {
    case device::FidoRequestHandlerBase::BleStatus::kOn:
      break;
    case device::FidoRequestHandlerBase::BleStatus::kOff:
      if (can_power_on_ble_adapter_ && bluetooth_adapter_power_on_callback_) {
        bluetooth_adapter_power_on_callback_.Run();
      }
      break;
    case device::FidoRequestHandlerBase::BleStatus::kPermissionDenied:
      FIDO_LOG(ERROR) << "Bluetooth permission denied; hybrid transport "
                         "cannot discover the phone.";
      break;
    case device::FidoRequestHandlerBase::BleStatus::kPendingPermissionRequest:
      break;
  }
}

void ElectronAuthenticatorRequestClientDelegate::
    MaybeEmitSelectAuthenticatorEvent() {
  if (pending_authenticators_.size() == 1) {
    // Run may destroy |this|; consume member state first.
    std::string id = std::move(pending_authenticators_[0].id);
    pending_authenticators_.clear();
    auto callback = request_callback_;
    callback.Run(id);
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

  // A listener that runs the callback synchronously dispatches the request,
  // which may destroy |this| before EmitEvent returns.
  base::WeakPtr<ElectronAuthenticatorRequestClientDelegate> weak_this =
      weak_factory_.GetWeakPtr();
  v8::Local<v8::Value> emit_result = gin_helper::EmitEvent(
      isolate, session_wrapper, "select-webauthn-authenticator", event_object,
      base::BindRepeating(
          &ElectronAuthenticatorRequestClientDelegate::OnAuthenticatorSelected,
          weak_factory_.GetWeakPtr()));
  if (!weak_this) {
    return;
  }

  bool had_listener = false;
  if (!gin::ConvertFromV8(isolate, emit_result, &had_listener) ||
      !had_listener) {
    DispatchDefaultAuthenticator();
  }
}

void ElectronAuthenticatorRequestClientDelegate::
    DispatchDefaultAuthenticator() {
  // A listener may have consumed the selection synchronously and then thrown.
  if (pending_authenticators_.empty()) {
    return;
  }
  auto auth = std::ranges::find(pending_authenticators_, "platformPasskeys",
                                &PendingAuthenticator::display_name);
  // Run may destroy |this|; consume member state first.
  std::string id = std::move(auth != pending_authenticators_.end()
                                 ? auth->id
                                 : pending_authenticators_.front().id);
  pending_authenticators_.clear();
  auto callback = request_callback_;
  callback.Run(id);
}

void ElectronAuthenticatorRequestClientDelegate::OnAuthenticatorSelected(
    gin::Arguments* args) {
  // Repeating callback: ignore any call after the first.
  if (pending_authenticators_.empty()) {
    return;
  }

  std::string selected_name;
  const bool has_name = args->GetNext(&selected_name) && !selected_name.empty();
  const auto selected =
      has_name ? std::ranges::find(pending_authenticators_, selected_name,
                                   &PendingAuthenticator::display_name)
               : pending_authenticators_.end();

  // Run may destroy |this|; consume member state first.
  if (selected != pending_authenticators_.end()) {
    std::string id = std::move(selected->id);
    pending_authenticators_.clear();
    auto callback = request_callback_;
    callback.Run(id);
    return;
  }

  // No argument, empty, or unknown name: cancel.
  pending_authenticators_.clear();
  if (cancel_callback_) {
    std::move(cancel_callback_).Run();
  }
}

#if BUILDFLAG(IS_MAC)
std::vector<std::unique_ptr<device::FidoDiscoveryBase>>
ElectronAuthenticatorRequestClientDelegate::CreatePlatformDiscoveries() {
  std::vector<std::unique_ptr<device::FidoDiscoveryBase>> discoveries;
  if (ElectronWebAuthenticationDelegate::IsPlatformPasskeysEnabled()) {
    auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
    if (rfh && IsSameOriginWithAncestors(rfh)) {
      discoveries.push_back(
          std::make_unique<ElectronPlatformPasskeysDiscovery>(rfh));
    } else if (rfh) {
      // Apple's API can't serve cross-origin ceremonies; other authenticators
      // still can.
      rfh->AddMessageToConsole(
          blink::mojom::ConsoleMessageLevel::kWarning,
          "WebAuthn platform passkeys are unavailable to cross-origin "
          "(iframe) requests: Apple's ASAuthorizationController cannot "
          "fulfill them. Other authenticators can still serve the request; "
          "the Touch ID authenticator supports iframes.");
    }
  }
  return discoveries;
}
#endif

}  // namespace electron
