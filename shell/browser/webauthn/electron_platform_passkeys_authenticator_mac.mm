// Copyright (c) 2026 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// This file drives passkey operations via Apple's ASAuthorizationController
// API. We don't reuse Chromium's device/fido/mac/icloud_keychain.mm because it
// gates on the com.apple.developer.web-browser.public-key-credential
// entitlement (checked via ProcessHasEntitlement) and is deeply coupled to
// Chrome's dialog model. Electron apps don't (and can't) carry that
// entitlement, but ASAuthorizationController works without it for any app that
// declares an Associated Domain with the "webcredentials:" service.

#include "shell/browser/webauthn/electron_platform_passkeys_authenticator.h"

#import <AuthenticationServices/AuthenticationServices.h>

#include <algorithm>
#include <utility>

#include "base/base64url.h"
#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "components/cbor/reader.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "device/fido/attestation_object.h"
#include "device/fido/authenticator_data.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/authenticator_supported_options.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/ctap_make_credential_request.h"
#include "device/fido/fido_request_handler_base.h"
#include "device/fido/public/fido_constants.h"
#include "device/fido/public/fido_types.h"
#include "device/fido/public/public_key_credential_descriptor.h"
#include "device/fido/public/public_key_credential_params.h"
#include "device/fido/public/public_key_credential_user_entity.h"
#include "shell/browser/native_window.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"

namespace electron {

namespace {

NSData* VectorToNSData(const std::vector<uint8_t>& vec) {
  return [NSData dataWithBytes:vec.data() length:vec.size()];
}

std::vector<uint8_t> NSDataToVector(NSData* data) {
  if (!data)
    return {};
  // SAFETY: NSData guarantees `bytes` points to `length` contiguous bytes.
  return UNSAFE_BUFFERS(std::vector<uint8_t>(
      static_cast<const uint8_t*>(data.bytes),
      static_cast<const uint8_t*>(data.bytes) + data.length));
}

// The fields we need out of Chromium's clientDataJSON before dispatching to
// Apple. `valid` is false when the JSON couldn't be parsed or lacked a
// decodable challenge — callers must reject the request rather than fall back
// to client_data_hash (Apple would hash that again, so the assertion could
// never verify RP-side).
struct ParsedClientData {
  bool valid = false;
  std::vector<uint8_t> challenge;
  // True for iframe ceremonies. Apple's public API always produces a top-level
  // `origin: https://<rpId>` clientDataJSON with no crossOrigin, so we can't
  // faithfully represent a cross-origin request and must reject it instead.
  bool cross_origin = false;
};

ParsedClientData ParseClientData(const std::string& client_data_json) {
  ParsedClientData result;
  if (client_data_json.empty())
    return result;
  std::optional<base::DictValue> parsed =
      base::JSONReader::ReadDict(client_data_json, /*options=*/0);
  if (!parsed)
    return result;
  const std::string* challenge_b64url = parsed->FindString("challenge");
  if (!challenge_b64url)
    return result;
  std::optional<std::vector<uint8_t>> challenge_bytes = base::Base64UrlDecode(
      *challenge_b64url, base::Base64UrlDecodePolicy::DISALLOW_PADDING);
  if (!challenge_bytes)
    return result;
  result.valid = true;
  result.challenge = std::move(*challenge_bytes);
  result.cross_origin = parsed->FindBool("crossOrigin").value_or(false);
  return result;
}

NSWindow* GetNSWindowForRenderFrameHost(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host)
    return nil;
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return nil;
  auto* relay = NativeWindowRelay::FromWebContents(web_contents);
  if (!relay || !relay->GetNativeWindow())
    return nil;
  return relay->GetNativeWindow()->GetNativeWindow().GetNativeNSWindow();
}

device::AuthenticatorSupportedOptions BuildOptions() {
  device::AuthenticatorSupportedOptions options;
  options.is_platform_device =
      device::AuthenticatorSupportedOptions::PlatformDevice::kYes;
  options.supports_resident_key = true;
  options.user_verification_availability =
      device::AuthenticatorSupportedOptions::UserVerificationAvailability::
          kSupportedAndConfigured;
  options.supports_user_presence = true;
  return options;
}

// Validates that Apple's returned clientDataJSON contains the expected
// challenge, origin, and type. This is a defense-in-depth check: we passed
// Apple the challenge and it derives the origin from the app's associated
// domain, so the JSON should echo the values we expect. Callers fail the
// ceremony when this returns false rather than forward unexpected bytes to the
// page.
bool ValidateAppleClientDataJSON(const std::vector<uint8_t>& raw_json,
                                 const std::vector<uint8_t>& expected_challenge,
                                 const std::string& expected_origin,
                                 const std::string& expected_type) {
  if (raw_json.empty() || expected_challenge.empty() ||
      expected_origin.empty() || expected_type.empty()) {
    return false;
  }

  std::string json_str(raw_json.begin(), raw_json.end());
  std::optional<base::DictValue> parsed =
      base::JSONReader::ReadDict(json_str, /*options=*/0);
  if (!parsed) {
    return false;
  }

  // Validate type matches the expected ceremony.
  const std::string* type = parsed->FindString("type");
  if (!type || *type != expected_type) {
    return false;
  }

  // Validate origin matches what Chromium computed for this frame.
  const std::string* origin = parsed->FindString("origin");
  if (!origin || *origin != expected_origin) {
    return false;
  }

  // Validate challenge round-trips correctly. Apple encodes it as base64url.
  const std::string* challenge_b64url = parsed->FindString("challenge");
  if (!challenge_b64url) {
    return false;
  }
  std::optional<std::vector<uint8_t>> decoded_challenge = base::Base64UrlDecode(
      *challenge_b64url, base::Base64UrlDecodePolicy::DISALLOW_PADDING);
  if (!decoded_challenge || *decoded_challenge != expected_challenge) {
    return false;
  }

  // Validate crossOrigin is not unexpectedly true. Apple should set it to
  // false (or omit it) when we don't request cross-origin.
  const std::optional<bool> cross_origin = parsed->FindBool("crossOrigin");
  if (cross_origin.has_value() && *cross_origin) {
    // Apple claims cross-origin but we didn't request it. Reject.
    return false;
  }

  return true;
}

}  // namespace

}  // namespace electron

// MARK: - ObjC Delegate

API_AVAILABLE(macos(13.0))
@interface ElectronPlatformPasskeyDelegate
    : NSObject <ASAuthorizationControllerDelegate,
                ASAuthorizationControllerPresentationContextProviding>

@property(nonatomic, weak) NSWindow* presentationAnchor;
@property(nonatomic, copy) void (^creationHandler)
    (NSError*, ASAuthorizationPlatformPublicKeyCredentialRegistration*);
@property(nonatomic, copy) void (^assertionHandler)
    (NSError*, ASAuthorizationPlatformPublicKeyCredentialAssertion*);

@end

API_AVAILABLE(macos(13.0))
@implementation ElectronPlatformPasskeyDelegate

@synthesize presentationAnchor = _presentationAnchor;
@synthesize creationHandler = _creationHandler;
@synthesize assertionHandler = _assertionHandler;

- (ASPresentationAnchor)presentationAnchorForAuthorizationController:
    (ASAuthorizationController*)controller {
  return self.presentationAnchor;
}

- (void)authorizationController:(ASAuthorizationController*)controller
    didCompleteWithAuthorization:(ASAuthorization*)authorization {
  id credential = authorization.credential;

  if ([credential
          isKindOfClass:[ASAuthorizationPlatformPublicKeyCredentialRegistration
                            class]]) {
    auto* registration =
        (ASAuthorizationPlatformPublicKeyCredentialRegistration*)credential;
    if (self.creationHandler) {
      self.creationHandler(nil, registration);
      self.creationHandler = nil;
    }
  } else if ([credential
                 isKindOfClass:
                     [ASAuthorizationPlatformPublicKeyCredentialAssertion
                         class]]) {
    auto* assertion =
        (ASAuthorizationPlatformPublicKeyCredentialAssertion*)credential;
    if (self.assertionHandler) {
      self.assertionHandler(nil, assertion);
      self.assertionHandler = nil;
    }
  }
}

- (void)authorizationController:(ASAuthorizationController*)controller
           didCompleteWithError:(NSError*)error {
  if (self.creationHandler) {
    self.creationHandler(error, nil);
    self.creationHandler = nil;
  }
  if (self.assertionHandler) {
    self.assertionHandler(error, nil);
    self.assertionHandler = nil;
  }
}

@end

namespace electron {

// MARK: - ObjCStorage

struct ElectronPlatformPasskeysAuthenticator::ObjCStorage {
  ElectronPlatformPasskeyDelegate* __strong delegate
      API_AVAILABLE(macos(13.0)) = nil;
  ASAuthorizationController* __strong controller API_AVAILABLE(macos(13.0)) =
      nil;

  // Result fields populated by the delegate callback, consumed by OnXxxComplete
  bool succeeded = false;
  NSInteger error_code = 0;
  bool error_is_authorization_domain = false;
  std::vector<uint8_t> credential_id;
  std::vector<uint8_t> raw_attestation_object;
  std::vector<uint8_t> raw_authenticator_data;
  std::vector<uint8_t> signature;
  std::vector<uint8_t> user_handle;
  // Apple's rawClientDataJSON, read from the public ASPublicKeyCredential
  // getter.
  std::vector<uint8_t> raw_client_data_json;

  // Expected values for validating Apple's returned clientDataJSON, checked in
  // OnXxxComplete before accepting raw_client_data_json as the override.
  std::vector<uint8_t> expected_challenge;
  std::string expected_origin;
  std::string expected_type;  // "webauthn.create" or "webauthn.get"
};

// MARK: - Authenticator Implementation

ElectronPlatformPasskeysAuthenticator::ElectronPlatformPasskeysAuthenticator(
    content::GlobalRenderFrameHostToken render_frame_host_token)
    : render_frame_host_token_(render_frame_host_token),
      objc_storage_(std::make_unique<ObjCStorage>()) {}

ElectronPlatformPasskeysAuthenticator::
    ~ElectronPlatformPasskeysAuthenticator() {
  Cancel();
}

// static
bool ElectronPlatformPasskeysAuthenticator::IsAvailable() {
  if (@available(macOS 13.0, *)) {
    return true;
  }
  return false;
}

void ElectronPlatformPasskeysAuthenticator::InitializeAuthenticator(
    base::OnceClosure callback) {
  std::move(callback).Run();
}

void ElectronPlatformPasskeysAuthenticator::MakeCredential(
    device::CtapMakeCredentialRequest request,
    device::MakeCredentialOptions options,
    MakeCredentialCallback callback) {
  if (@available(macOS 13.0, *)) {
    Cancel();
    // Start from a clean slate so state from a prior op on this authenticator
    // (expected_challenge/origin/type, buffers) can't bleed into this one.
    objc_storage_ = std::make_unique<ObjCStorage>();

    content::RenderFrameHost* rfh =
        content::RenderFrameHost::FromFrameToken(render_frame_host_token_);
    if (!rfh) {
      std::move(callback).Run(device::MakeCredentialStatus::kUserConsentDenied,
                              std::nullopt);
      return;
    }

    NSWindow* window = GetNSWindowForRenderFrameHost(rfh);
    if (!window) {
      std::move(callback).Run(device::MakeCredentialStatus::kUserConsentDenied,
                              std::nullopt);
      return;
    }

    // The public ASAuthorization API ignores pubKeyCredParams and always mints
    // an ES256 (-7) credential. If the RP doesn't accept ES256, creating a
    // credential here would produce one it immediately rejects, so fail early.
    const auto& params =
        request.public_key_credential_params.public_key_credential_params();
    const bool supports_es256 = std::ranges::any_of(params, [](const auto& p) {
      return p.algorithm == base::strict_cast<int32_t>(
                                device::CoseAlgorithmIdentifier::kEs256);
    });
    if (!supports_es256) {
      std::move(callback).Run(device::MakeCredentialStatus::kNoCommonAlgorithms,
                              std::nullopt);
      return;
    }

    // Hand Apple the real challenge (not client_data_hash) so it builds a
    // clientDataJSON — with our challenge and the associated-domain origin —
    // and signs over its hash, letting the RP verify against the
    // rawClientDataJSON we return. We deliberately do NOT set the private
    // `clientData` property: the public API already produces a verifiable
    // clientDataJSON for a domain-associated app, and the KVC setter is Apple
    // SPI that App Store review rejects.
    ParsedClientData client_data = ParseClientData(request.client_data_json);
    if (!client_data.valid) {
      // Falling back to client_data_hash here would let Apple hash it again,
      // producing a signature no RP can verify. Reject instead.
      std::move(callback).Run(
          device::MakeCredentialStatus::kAuthenticatorResponseInvalid,
          std::nullopt);
      return;
    }
    if (client_data.cross_origin) {
      // Apple's public API always builds a top-level `origin: https://<rpId>`
      // clientDataJSON, so we can't represent a cross-origin (iframe) ceremony.
      // Reject rather than silently present it to the RP as same-origin.
      std::move(callback).Run(
          device::MakeCredentialStatus::kAuthenticatorResponseInvalid,
          std::nullopt);
      return;
    }

    auto* provider = [[ASAuthorizationPlatformPublicKeyCredentialProvider alloc]
        initWithRelyingPartyIdentifier:base::SysUTF8ToNSString(request.rp.id)];

    auto* as_request = [provider
        createCredentialRegistrationRequestWithChallenge:VectorToNSData(
                                                             client_data
                                                                 .challenge)
                                                    name:
                                                        base::SysUTF8ToNSString(
                                                            request.user.name
                                                                .value_or(""))
                                                  userID:VectorToNSData(
                                                             request.user.id)];

    // Stash expected values so OnMakeCredentialComplete can validate Apple's
    // returned clientDataJSON before accepting it as the override. Apple
    // derives the origin from the associated domain as https://<rpId>.
    objc_storage_->expected_challenge = std::move(client_data.challenge);
    objc_storage_->expected_origin = "https://" + request.rp.id;
    objc_storage_->expected_type = "webauthn.create";

    // excludedCredentials comes from the
    // ASAuthorizationWebBrowserPlatformPublicKeyCredentialRegistrationRequest
    // protocol extension, which is macOS 13.5+. The availability lives on the
    // protocol rather than the property, so clang won't flag it inside the 13.0
    // guard; on 13.0-13.4 setting it would be an unrecognized selector.
    if (!request.exclude_list.empty()) {
      if (@available(macOS 13.5, *)) {
        NSMutableArray* descriptors = [NSMutableArray array];
        for (const auto& excluded : request.exclude_list) {
          auto* descriptor =
              [[ASAuthorizationPlatformPublicKeyCredentialDescriptor alloc]
                  initWithCredentialID:VectorToNSData(excluded.id)];
          [descriptors addObject:descriptor];
        }
        as_request.excludedCredentials = descriptors;
      }
    }

    objc_storage_->delegate = [[ElectronPlatformPasskeyDelegate alloc] init];
    objc_storage_->delegate.presentationAnchor = window;

    __block MakeCredentialCallback moved_callback = std::move(callback);
    __block auto weak_self = weak_factory_.GetWeakPtr();
    objc_storage_->delegate.creationHandler = ^(
        NSError* error,
        ASAuthorizationPlatformPublicKeyCredentialRegistration* registration) {
      if (!weak_self)
        return;
      auto& storage = weak_self->objc_storage_;
      if (error || !registration) {
        storage->succeeded = false;
        storage->error_code = error ? error.code : 0;
        storage->error_is_authorization_domain =
            error && [error.domain isEqualToString:ASAuthorizationErrorDomain];
      } else {
        storage->succeeded = true;
        storage->credential_id = NSDataToVector(registration.credentialID);
        storage->raw_attestation_object =
            NSDataToVector(registration.rawAttestationObject);
        storage->raw_client_data_json =
            NSDataToVector(registration.rawClientDataJSON);
      }
      weak_self->OnMakeCredentialComplete(std::move(moved_callback));
    };

    objc_storage_->controller = [[ASAuthorizationController alloc]
        initWithAuthorizationRequests:@[ as_request ]];
    objc_storage_->controller.delegate = objc_storage_->delegate;
    objc_storage_->controller.presentationContextProvider =
        objc_storage_->delegate;
    [objc_storage_->controller performRequests];
  } else {
    std::move(callback).Run(device::MakeCredentialStatus::kUserConsentDenied,
                            std::nullopt);
  }
}

void ElectronPlatformPasskeysAuthenticator::GetAssertion(
    device::CtapGetAssertionRequest request,
    device::CtapGetAssertionOptions options,
    GetAssertionCallback callback) {
  if (@available(macOS 13.0, *)) {
    Cancel();
    // Start from a clean slate so state from a prior op on this authenticator
    // (expected_challenge/origin/type, buffers) can't bleed into this one.
    objc_storage_ = std::make_unique<ObjCStorage>();

    content::RenderFrameHost* rfh =
        content::RenderFrameHost::FromFrameToken(render_frame_host_token_);
    if (!rfh) {
      std::move(callback).Run(device::GetAssertionStatus::kUserConsentDenied,
                              {});
      return;
    }

    NSWindow* window = GetNSWindowForRenderFrameHost(rfh);
    if (!window) {
      std::move(callback).Run(device::GetAssertionStatus::kUserConsentDenied,
                              {});
      return;
    }

    // Hand Apple the real challenge (not client_data_hash) so it builds a
    // clientDataJSON — with our challenge and the associated-domain origin —
    // and signs over its hash, letting the RP verify against the
    // rawClientDataJSON we return. As in MakeCredential, we use only the public
    // API and never set the private `clientData` property (Apple SPI that App
    // Store review rejects).
    ParsedClientData client_data = ParseClientData(request.client_data_json);
    if (!client_data.valid) {
      // Falling back to client_data_hash here would let Apple hash it again,
      // producing a signature no RP can verify. Reject instead.
      std::move(callback).Run(
          device::GetAssertionStatus::kAuthenticatorResponseInvalid, {});
      return;
    }
    if (client_data.cross_origin) {
      // Apple's public API always builds a top-level `origin: https://<rpId>`
      // clientDataJSON, so we can't represent a cross-origin (iframe) ceremony.
      // Reject rather than silently present it to the RP as same-origin.
      std::move(callback).Run(
          device::GetAssertionStatus::kAuthenticatorResponseInvalid, {});
      return;
    }

    auto* provider = [[ASAuthorizationPlatformPublicKeyCredentialProvider alloc]
        initWithRelyingPartyIdentifier:base::SysUTF8ToNSString(request.rp_id)];

    auto* as_request = [provider
        createCredentialAssertionRequestWithChallenge:VectorToNSData(
                                                          client_data
                                                              .challenge)];

    // Stash expected values so OnGetAssertionComplete can validate Apple's
    // returned clientDataJSON before accepting it as the override. Apple
    // derives the origin from the associated domain as https://<rpId>.
    objc_storage_->expected_challenge = std::move(client_data.challenge);
    objc_storage_->expected_origin = "https://" + request.rp_id;
    objc_storage_->expected_type = "webauthn.get";

    if (!request.allow_list.empty()) {
      NSMutableArray* descriptors = [NSMutableArray array];
      for (const auto& allowed : request.allow_list) {
        auto* descriptor =
            [[ASAuthorizationPlatformPublicKeyCredentialDescriptor alloc]
                initWithCredentialID:VectorToNSData(allowed.id)];
        [descriptors addObject:descriptor];
      }
      as_request.allowedCredentials = descriptors;
    }

    objc_storage_->delegate = [[ElectronPlatformPasskeyDelegate alloc] init];
    objc_storage_->delegate.presentationAnchor = window;

    __block GetAssertionCallback moved_callback = std::move(callback);
    __block auto weak_self = weak_factory_.GetWeakPtr();
    objc_storage_->delegate.assertionHandler = ^(
        NSError* error,
        ASAuthorizationPlatformPublicKeyCredentialAssertion* assertion) {
      if (!weak_self)
        return;
      auto& storage = weak_self->objc_storage_;
      if (error || !assertion) {
        storage->succeeded = false;
        storage->error_code = error ? error.code : 0;
        storage->error_is_authorization_domain =
            error && [error.domain isEqualToString:ASAuthorizationErrorDomain];
      } else {
        storage->succeeded = true;
        storage->credential_id = NSDataToVector(assertion.credentialID);
        storage->raw_authenticator_data =
            NSDataToVector(assertion.rawAuthenticatorData);
        storage->signature = NSDataToVector(assertion.signature);
        storage->user_handle = NSDataToVector(assertion.userID);
        storage->raw_client_data_json =
            NSDataToVector(assertion.rawClientDataJSON);
      }
      weak_self->OnGetAssertionComplete(std::move(moved_callback));
    };

    objc_storage_->controller = [[ASAuthorizationController alloc]
        initWithAuthorizationRequests:@[ as_request ]];
    objc_storage_->controller.delegate = objc_storage_->delegate;
    objc_storage_->controller.presentationContextProvider =
        objc_storage_->delegate;
    [objc_storage_->controller performRequests];
  } else {
    std::move(callback).Run(device::GetAssertionStatus::kUserConsentDenied, {});
  }
}

void ElectronPlatformPasskeysAuthenticator::GetPlatformCredentialInfoForRequest(
    const device::CtapGetAssertionRequest& request,
    const device::CtapGetAssertionOptions& options,
    GetPlatformCredentialInfoForRequestCallback callback) {
  // ASAuthorizationController doesn't expose a pre-flight query API, so we
  // can't enumerate matching credentials. Report kUnknown to let the request
  // proceed — the actual credential lookup happens during GetAssertion.
  std::move(callback).Run(
      {}, device::FidoRequestHandlerBase::RecognizedCredential::kUnknown);
}

void ElectronPlatformPasskeysAuthenticator::Cancel() {
  if (@available(macOS 13.0, *)) {
    if (objc_storage_->controller) {
      [objc_storage_->controller cancel];
    }
    objc_storage_->controller = nil;
    objc_storage_->delegate = nil;
  }
}

device::AuthenticatorType ElectronPlatformPasskeysAuthenticator::GetType()
    const {
  return device::AuthenticatorType::kICloudKeychain;
}

std::string ElectronPlatformPasskeysAuthenticator::GetId() const {
  return "ElectronPlatformPasskeysAuthenticator";
}

const device::AuthenticatorSupportedOptions&
ElectronPlatformPasskeysAuthenticator::Options() const {
  static const base::NoDestructor<device::AuthenticatorSupportedOptions>
      options(BuildOptions());
  return *options;
}

std::optional<device::FidoTransportProtocol>
ElectronPlatformPasskeysAuthenticator::AuthenticatorTransport() const {
  return device::FidoTransportProtocol::kInternal;
}

base::WeakPtr<device::FidoAuthenticator>
ElectronPlatformPasskeysAuthenticator::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void ElectronPlatformPasskeysAuthenticator::MaybeLogUnconfiguredError() {
  // Error 1004 is the "no application identifier" / association failure the
  // system raises when the app isn't signed with the associated-domains
  // entitlement, or the relying party's domain isn't associated via an
  // apple-app-site-association file. It surfaces to the page as a bare
  // NotAllowedError with no hint, so log actionable guidance.
  constexpr NSInteger kNoApplicationIdentifier = 1004;
  if (!objc_storage_->error_is_authorization_domain ||
      objc_storage_->error_code != kNoApplicationIdentifier) {
    return;
  }
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromFrameToken(render_frame_host_token_);
  if (!rfh) {
    return;
  }
  rfh->AddMessageToConsole(
      blink::mojom::ConsoleMessageLevel::kError,
      "WebAuthn platform passkey request failed (ASAuthorizationError 1004). "
      "This usually means the app isn't signed with the associated-domains "
      "entitlement, or the relying party's domain isn't serving a valid "
      ".well-known/apple-app-site-association with a \"webcredentials\" entry "
      "for this app. Platform passkeys require a real associated domain (there "
      "is no localhost rpId); development-signed builds can test against one "
      "via the entitlement's ?mode=developer modifier.");
}

void ElectronPlatformPasskeysAuthenticator::OnMakeCredentialComplete(
    MakeCredentialCallback callback) {
  if (!objc_storage_->succeeded) {
    device::MakeCredentialStatus status =
        device::MakeCredentialStatus::kUserConsentDenied;

    // ASAuthorizationErrorMatchedExcludedCredential (1006) was added in
    // macOS 14. Use the raw value to compile against older SDKs.
    constexpr NSInteger kMatchedExcludedCredential = 1006;
    if (objc_storage_->error_is_authorization_domain &&
        objc_storage_->error_code == kMatchedExcludedCredential) {
      status = device::MakeCredentialStatus::kUserConsentButCredentialExcluded;
    }

    MaybeLogUnconfiguredError();
    std::move(callback).Run(status, std::nullopt);
    return;
  }

  std::optional<cbor::Value> cbor_value =
      cbor::Reader::Read(objc_storage_->raw_attestation_object);
  if (!cbor_value) {
    std::move(callback).Run(
        device::MakeCredentialStatus::kAuthenticatorResponseInvalid,
        std::nullopt);
    return;
  }

  std::optional<device::AttestationObject> attestation_object =
      device::AttestationObject::Parse(*cbor_value);
  if (!attestation_object) {
    std::move(callback).Run(
        device::MakeCredentialStatus::kAuthenticatorResponseInvalid,
        std::nullopt);
    return;
  }

  device::AuthenticatorMakeCredentialResponse response(
      device::FidoTransportProtocol::kInternal, std::move(*attestation_object));
  response.is_resident_key = true;
  response.transports.emplace();
  response.transports->insert(device::FidoTransportProtocol::kInternal);
  response.transports->insert(device::FidoTransportProtocol::kHybrid);
  response.transport_used = device::FidoTransportProtocol::kInternal;

  // Verify Apple's returned JSON echoes the challenge, origin, and type we
  // requested, then hand it to the page. A registration that reaches this point
  // has already minted a real passkey in the user's keychain; if Apple's JSON
  // is unexpectedly absent or fails validation we must fail the ceremony rather
  // than forward Chromium's JSON — the latter would let create() "succeed"
  // (attestation is `none`) while every future assertion against that new
  // credential fails RP-side.
  if (!ValidateAppleClientDataJSON(objc_storage_->raw_client_data_json,
                                   objc_storage_->expected_challenge,
                                   objc_storage_->expected_origin,
                                   objc_storage_->expected_type)) {
    std::move(callback).Run(
        device::MakeCredentialStatus::kAuthenticatorResponseInvalid,
        std::nullopt);
    return;
  }
  response.raw_client_data_json =
      std::move(objc_storage_->raw_client_data_json);

  std::move(callback).Run(device::MakeCredentialStatus::kSuccess,
                          std::move(response));
}

void ElectronPlatformPasskeysAuthenticator::OnGetAssertionComplete(
    GetAssertionCallback callback) {
  if (!objc_storage_->succeeded) {
    MaybeLogUnconfiguredError();
    std::move(callback).Run(device::GetAssertionStatus::kUserConsentDenied, {});
    return;
  }

  std::optional<device::AuthenticatorData> auth_data =
      device::AuthenticatorData::DecodeAuthenticatorData(
          base::span<const uint8_t>(objc_storage_->raw_authenticator_data));
  if (!auth_data) {
    std::move(callback).Run(
        device::GetAssertionStatus::kAuthenticatorResponseInvalid, {});
    return;
  }

  device::AuthenticatorGetAssertionResponse response(
      std::move(*auth_data), std::move(objc_storage_->signature),
      device::FidoTransportProtocol::kInternal);

  response.credential = device::PublicKeyCredentialDescriptor(
      device::CredentialType::kPublicKey,
      std::move(objc_storage_->credential_id));

  if (!objc_storage_->user_handle.empty()) {
    response.user_entity = device::PublicKeyCredentialUserEntity(
        std::move(objc_storage_->user_handle));
  }
  response.user_selected = true;

  // Apple's rawClientDataJSON is the authoritative clientDataJSON — the
  // signature covers its hash — so the page must receive it for the RP to
  // verify. Validate it echoes what we requested; if it's absent or fails
  // validation, fail the ceremony rather than forward Chromium's JSON, which
  // wouldn't match the signature and would fail RP-side anyway.
  if (!ValidateAppleClientDataJSON(objc_storage_->raw_client_data_json,
                                   objc_storage_->expected_challenge,
                                   objc_storage_->expected_origin,
                                   objc_storage_->expected_type)) {
    std::move(callback).Run(
        device::GetAssertionStatus::kAuthenticatorResponseInvalid, {});
    return;
  }
  response.raw_client_data_json =
      std::move(objc_storage_->raw_client_data_json);

  std::vector<device::AuthenticatorGetAssertionResponse> responses;
  responses.push_back(std::move(response));
  std::move(callback).Run(device::GetAssertionStatus::kSuccess,
                          std::move(responses));
}

}  // namespace electron
