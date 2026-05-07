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

#include <utility>

#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/sys_string_conversions.h"
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
#include "device/fido/public/public_key_credential_user_entity.h"
#include "shell/browser/native_window.h"

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

    auto* provider = [[ASAuthorizationPlatformPublicKeyCredentialProvider alloc]
        initWithRelyingPartyIdentifier:base::SysUTF8ToNSString(request.rp.id)];

    NSData* challenge = [NSData dataWithBytes:request.client_data_hash.data()
                                       length:request.client_data_hash.size()];

    auto* as_request = [provider
        createCredentialRegistrationRequestWithChallenge:challenge
                                                    name:
                                                        base::SysUTF8ToNSString(
                                                            request.user.name
                                                                .value_or(""))
                                                  userID:VectorToNSData(
                                                             request.user.id)];

    if (!request.exclude_list.empty()) {
      NSMutableArray* descriptors = [NSMutableArray array];
      for (const auto& excluded : request.exclude_list) {
        auto* descriptor =
            [[ASAuthorizationPlatformPublicKeyCredentialDescriptor alloc]
                initWithCredentialID:VectorToNSData(excluded.id)];
        [descriptors addObject:descriptor];
      }
      as_request.excludedCredentials = descriptors;
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

    auto* provider = [[ASAuthorizationPlatformPublicKeyCredentialProvider alloc]
        initWithRelyingPartyIdentifier:base::SysUTF8ToNSString(request.rp_id)];

    NSData* challenge = [NSData dataWithBytes:request.client_data_hash.data()
                                       length:request.client_data_hash.size()];

    auto* as_request =
        [provider createCredentialAssertionRequestWithChallenge:challenge];

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

  std::move(callback).Run(device::MakeCredentialStatus::kSuccess,
                          std::move(response));
}

void ElectronPlatformPasskeysAuthenticator::OnGetAssertionComplete(
    GetAssertionCallback callback) {
  if (!objc_storage_->succeeded) {
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

  std::vector<device::AuthenticatorGetAssertionResponse> responses;
  responses.push_back(std::move(response));
  std::move(callback).Run(device::GetAssertionStatus::kSuccess,
                          std::move(responses));
}

}  // namespace electron
