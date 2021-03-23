// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webauthn/chrome_authenticator_request_delegate.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/feature_list.h"
#include "base/location.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/ui/webauthn/authenticator_request_dialog.h"
#include "chrome/browser/webauthn/authenticator_request_dialog_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/device_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "crypto/random.h"
#include "device/fido/cable/v2_handshake.h"
#include "device/fido/features.h"
#include "device/fido/fido_authenticator.h"
#include "device/fido/fido_discovery_factory.h"
#include "shell/browser/electron_browser_context.h"

#if defined(OS_MAC)
#include "device/fido/mac/authenticator.h"
#include "device/fido/mac/credential_metadata.h"
#endif

#if defined(OS_WIN)
#include "device/fido/win/authenticator.h"
#endif

namespace {

// Returns true iff |relying_party_id| is listed in the
// SecurityKeyPermitAttestation policy.
bool IsWebauthnRPIDListedInEnterprisePolicy(
    content::BrowserContext* browser_context,
    const std::string& relying_party_id) {
  return false;
}

std::string Base64(base::span<const uint8_t> in) {
  std::string ret;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(in.data()), in.size()),
      &ret);
  return ret;
}

base::Optional<std::string> GetString(const base::Value& dict,
                                      const char* key) {
  const base::Value* v = dict.FindKey(key);
  if (!v || !v->is_string()) {
    return base::nullopt;
  }
  return v->GetString();
}

template <size_t N>
bool CopyBytestring(std::array<uint8_t, N>* out,
                    base::Optional<std::string> value) {
  if (!value) {
    return false;
  }

  std::string bytes;
  if (!base::Base64Decode(*value, &bytes) || bytes.size() != N) {
    return false;
  }

  std::copy(bytes.begin(), bytes.end(), out->begin());
  return true;
}

bool CopyBytestring(std::vector<uint8_t>* out,
                    base::Optional<std::string> value) {
  if (!value) {
    return false;
  }

  std::string bytes;
  if (!base::Base64Decode(*value, &bytes)) {
    return false;
  }

  out->clear();
  out->insert(out->begin(), bytes.begin(), bytes.end());
  return true;
}

bool CopyString(std::string* out, base::Optional<std::string> value) {
  if (!value) {
    return false;
  }
  *out = *value;
  return true;
}

#if defined(OS_MAC)
const char kWebAuthnTouchIdMetadataSecretPrefName[] =
    "webauthn.touchid.metadata_secret";
#endif

const char kWebAuthnLastTransportUsedPrefName[] =
    "webauthn.last_transport_used";

const char kWebAuthnCablePairingsPrefName[] = "webauthn.cablev2_pairings";

// The |kWebAuthnCablePairingsPrefName| preference contains a list of dicts,
// where each dict has these keys:
const char kPairingPrefName[] = "name";
const char kPairingPrefContactId[] = "contact_id";
const char kPairingPrefTunnelServer[] = "tunnel_server";
const char kPairingPrefId[] = "id";
const char kPairingPrefSecret[] = "secret";
const char kPairingPrefPublicKey[] = "pub_key";
const char kPairingPrefTime[] = "time";

// DeleteCablePairingByPublicKey erases any pairing with the given public key
// from |list|.
void DeleteCablePairingByPublicKey(base::ListValue* list,
                                   const std::string& public_key_base64) {
  list->EraseListValueIf([&public_key_base64](const auto& value) {
    if (!value.is_dict()) {
      return false;
    }
    const base::Value* pref_public_key = value.FindKey(kPairingPrefPublicKey);
    return pref_public_key && pref_public_key->is_string() &&
           pref_public_key->GetString() == public_key_base64;
  });
}

}  // namespace

// static
void ChromeAuthenticatorRequestDelegate::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
#if defined(OS_MAC)
  registry->RegisterStringPref(kWebAuthnTouchIdMetadataSecretPrefName,
                               std::string());
#endif

  registry->RegisterStringPref(kWebAuthnLastTransportUsedPrefName,
                               std::string());
  registry->RegisterListPref(kWebAuthnCablePairingsPrefName);
}

ChromeAuthenticatorRequestDelegate::ChromeAuthenticatorRequestDelegate(
    content::RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host) {}

ChromeAuthenticatorRequestDelegate::~ChromeAuthenticatorRequestDelegate() {
  // Currently, completion of the request is indicated by //content destroying
  // this delegate.
  if (weak_dialog_model_) {
    weak_dialog_model_->OnRequestComplete();
  }

  // The dialog model may be destroyed after the OnRequestComplete call.
  if (weak_dialog_model_) {
    weak_dialog_model_->RemoveObserver(this);
    weak_dialog_model_ = nullptr;
  }
}

base::WeakPtr<ChromeAuthenticatorRequestDelegate>
ChromeAuthenticatorRequestDelegate::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

AuthenticatorRequestDialogModel*
ChromeAuthenticatorRequestDelegate::WeakDialogModelForTesting() const {
  return weak_dialog_model_;
}

content::BrowserContext* ChromeAuthenticatorRequestDelegate::browser_context()
    const {
  return content::WebContents::FromRenderFrameHost(render_frame_host())
      ->GetBrowserContext();
}

base::Optional<std::string>
ChromeAuthenticatorRequestDelegate::MaybeGetRelyingPartyIdOverride(
    const std::string& claimed_relying_party_id,
    const url::Origin& caller_origin) {
  // Don't override cryptotoken processing.
  constexpr char kCryptotokenOrigin[] =
      "chrome-extension://kmendfapggjehodndflmmgagdbamhnfd";
  if (caller_origin == url::Origin::Create(GURL(kCryptotokenOrigin))) {
    return base::nullopt;
  }

  // Otherwise, allow extensions to use WebAuthn and map their origins directly
  // to RP IDs.
  if (caller_origin.scheme() == "chrome-extension") {
    // The requested RP ID for an extension must simply be the extension
    // identifier because no flexibility is permitted. If a caller doesn't
    // specify an RP ID then Blink defaults the value to the origin's host.
    if (claimed_relying_party_id != caller_origin.host()) {
      return base::nullopt;
    }
    return caller_origin.Serialize();
  }

  return base::nullopt;
}

void ChromeAuthenticatorRequestDelegate::SetRelyingPartyId(
    const std::string& rp_id) {
  transient_dialog_model_holder_ =
      std::make_unique<AuthenticatorRequestDialogModel>(rp_id);
  weak_dialog_model_ = transient_dialog_model_holder_.get();
}

bool ChromeAuthenticatorRequestDelegate::DoesBlockRequestOnFailure(
    InterestingFailureReason reason) {
  if (!IsWebAuthnUIEnabled())
    return false;
  if (!weak_dialog_model_)
    return false;

  switch (reason) {
    case InterestingFailureReason::kTimeout:
      weak_dialog_model_->OnRequestTimeout();
      break;
    case InterestingFailureReason::kKeyNotRegistered:
      weak_dialog_model_->OnActivatedKeyNotRegistered();
      break;
    case InterestingFailureReason::kKeyAlreadyRegistered:
      weak_dialog_model_->OnActivatedKeyAlreadyRegistered();
      break;
    case InterestingFailureReason::kSoftPINBlock:
      weak_dialog_model_->OnSoftPINBlock();
      break;
    case InterestingFailureReason::kHardPINBlock:
      weak_dialog_model_->OnHardPINBlock();
      break;
    case InterestingFailureReason::kAuthenticatorRemovedDuringPINEntry:
      weak_dialog_model_->OnAuthenticatorRemovedDuringPINEntry();
      break;
    case InterestingFailureReason::kAuthenticatorMissingResidentKeys:
      weak_dialog_model_->OnAuthenticatorMissingResidentKeys();
      break;
    case InterestingFailureReason::kAuthenticatorMissingUserVerification:
      weak_dialog_model_->OnAuthenticatorMissingUserVerification();
      break;
    case InterestingFailureReason::kAuthenticatorMissingLargeBlob:
      weak_dialog_model_->OnAuthenticatorMissingLargeBlob();
      break;
    case InterestingFailureReason::kNoCommonAlgorithms:
      weak_dialog_model_->OnNoCommonAlgorithms();
      break;
    case InterestingFailureReason::kStorageFull:
      weak_dialog_model_->OnAuthenticatorStorageFull();
      break;
    case InterestingFailureReason::kUserConsentDenied:
      weak_dialog_model_->OnUserConsentDenied();
      break;
    case InterestingFailureReason::kWinUserCancelled:
      return weak_dialog_model_->OnWinUserCancelled();
  }
  return true;
}

void ChromeAuthenticatorRequestDelegate::RegisterActionCallbacks(
    base::OnceClosure cancel_callback,
    base::RepeatingClosure start_over_callback,
    device::FidoRequestHandlerBase::RequestCallback request_callback,
    base::RepeatingClosure bluetooth_adapter_power_on_callback) {
  request_callback_ = request_callback;
  cancel_callback_ = std::move(cancel_callback);
  start_over_callback_ = std::move(start_over_callback);

  weak_dialog_model_->SetRequestCallback(request_callback);
  weak_dialog_model_->SetBluetoothAdapterPowerOnCallback(
      bluetooth_adapter_power_on_callback);
}

bool ChromeAuthenticatorRequestDelegate::ShouldPermitIndividualAttestation(
    const std::string& relying_party_id) {
  constexpr char kGoogleCorpAppId[] =
      "https://www.gstatic.com/securitykey/a/google.com/origins.json";

  // If the RP ID is actually the Google corp App ID (because the request is
  // actually a U2F request originating from cryptotoken), or is listed in the
  // enterprise policy, signal that individual attestation is permitted.
  return relying_party_id == kGoogleCorpAppId ||
         IsWebauthnRPIDListedInEnterprisePolicy(browser_context(),
                                                relying_party_id);
}

void ChromeAuthenticatorRequestDelegate::ShouldReturnAttestation(
    const std::string& relying_party_id,
    const device::FidoAuthenticator* authenticator,
    bool is_enterprise_attestation,
    base::OnceCallback<void(bool)> callback) {
  if (IsWebauthnRPIDListedInEnterprisePolicy(browser_context(),
                                             relying_party_id)) {
    // Enterprise attestations should have been approved already and not reach
    // this point.
    DCHECK(!is_enterprise_attestation);
    std::move(callback).Run(true);
    return;
  }

  // Cryptotoken displays its own attestation consent prompt.
  // AuthenticatorCommon does not invoke ShouldReturnAttestation() for those
  // requests.
  if (disable_ui_) {
    NOTREACHED();
    std::move(callback).Run(false);
    return;
  }

#if defined(OS_WIN)
  if (authenticator->IsWinNativeApiAuthenticator() &&
      static_cast<const device::WinWebAuthnApiAuthenticator*>(authenticator)
          ->ShowsPrivacyNotice()) {
    // The OS' native API includes an attestation prompt.
    std::move(callback).Run(true);
    return;
  }
#endif  // defined(OS_WIN)

  weak_dialog_model_->RequestAttestationPermission(is_enterprise_attestation,
                                                   std::move(callback));
}

bool ChromeAuthenticatorRequestDelegate::SupportsResidentKeys() {
  return true;
}

void ChromeAuthenticatorRequestDelegate::ConfigureCable(
    const url::Origin& origin,
    base::span<const device::CableDiscoveryData> pairings_from_extension,
    device::FidoDiscoveryFactory* discovery_factory) {
  std::vector<device::CableDiscoveryData> pairings;
  if (ShouldPermitCableExtension(origin)) {
    pairings.insert(pairings.end(), pairings_from_extension.begin(),
                    pairings_from_extension.end());
  }
  const bool cable_extension_provided = !pairings.empty();

  base::Optional<std::array<uint8_t, device::cablev2::kQRKeySize>>
      qr_generator_key;
  base::Optional<std::string> qr_string;
  bool have_paired_phones = false;
  std::vector<std::unique_ptr<device::cablev2::Pairing>> paired_phones;
  if (base::FeatureList::IsEnabled(device::kWebAuthPhoneSupport)) {
    qr_generator_key.emplace();
    crypto::RandBytes(*qr_generator_key);
    qr_string = device::cablev2::qr::Encode(*qr_generator_key);

    if (!cable_extension_provided) {
      paired_phones = GetCablePairings();
    }
    have_paired_phones = !paired_phones.empty();

    mojo::Remote<device::mojom::UsbDeviceManager> usb_device_manager;
    content::GetDeviceService().BindUsbDeviceManager(
        usb_device_manager.BindNewPipeAndPassReceiver());
    discovery_factory->set_usb_device_manager(std::move(usb_device_manager));
    discovery_factory->set_network_context(
        SystemNetworkContextManager::GetInstance()->GetContext());
  }

  if (!cable_extension_provided && !qr_generator_key) {
    return;
  }

  weak_dialog_model_->set_cable_transport_info(cable_extension_provided,
                                               have_paired_phones, qr_string);
  discovery_factory->set_cable_data(std::move(pairings), qr_generator_key,
                                    std::move(paired_phones));

  discovery_factory->set_cable_pairing_callback(base::BindRepeating(
      &ChromeAuthenticatorRequestDelegate::HandleCablePairingEvent,
      weak_ptr_factory_.GetWeakPtr()));
}

void ChromeAuthenticatorRequestDelegate::SelectAccount(
    std::vector<device::AuthenticatorGetAssertionResponse> responses,
    base::OnceCallback<void(device::AuthenticatorGetAssertionResponse)>
        callback) {
  if (disable_ui_) {
    // Cryptotoken requests should never reach account selection.
    NOTREACHED();
    std::move(cancel_callback_).Run();
    return;
  }

  if (!weak_dialog_model_) {
    std::move(cancel_callback_).Run();
    return;
  }

  weak_dialog_model_->SelectAccount(std::move(responses), std::move(callback));
}

bool ChromeAuthenticatorRequestDelegate::IsFocused() {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host());
  DCHECK(web_contents);
  return web_contents->GetVisibility() == content::Visibility::VISIBLE;
}

base::Optional<bool> ChromeAuthenticatorRequestDelegate::
    IsUserVerifyingPlatformAuthenticatorAvailableOverride() {
  return base::nullopt;
}

#if defined(OS_MAC)
static constexpr char kTouchIdKeychainAccessGroup[] =
    "EQHXZ8M8AV.com.google.Chrome.webauthn";

namespace {

std::string TouchIdMetadataSecret(electron::ElectronBrowserContext* profile) {
  PrefService* prefs = profile->prefs();
  std::string key = prefs->GetString(kWebAuthnTouchIdMetadataSecretPrefName);
  if (key.empty() || !base::Base64Decode(key, &key)) {
    key = device::fido::mac::GenerateCredentialMetadataSecret();
    std::string encoded_key;
    base::Base64Encode(key, &encoded_key);
    prefs->SetString(kWebAuthnTouchIdMetadataSecretPrefName, encoded_key);
  }
  return key;
}

}  // namespace

// static
ChromeAuthenticatorRequestDelegate::TouchIdAuthenticatorConfig
ChromeAuthenticatorRequestDelegate::TouchIdAuthenticatorConfigForProfile(
    electron::ElectronBrowserContext* profile) {
  return TouchIdAuthenticatorConfig{kTouchIdKeychainAccessGroup,
                                    TouchIdMetadataSecret(profile)};
}
#endif

void ChromeAuthenticatorRequestDelegate::UpdateLastTransportUsed(
    device::FidoTransportProtocol transport) {
  PrefService* prefs =
      electron::ElectronBrowserContext::FromBrowserContext(browser_context())
          ->prefs();
  prefs->SetString(kWebAuthnLastTransportUsedPrefName,
                   device::ToString(transport));
}

void ChromeAuthenticatorRequestDelegate::DisableUI() {
  disable_ui_ = true;
}

bool ChromeAuthenticatorRequestDelegate::IsWebAuthnUIEnabled() {
  // The UI is fully disabled for the entire request duration only if the
  // request originates from cryptotoken. The UI may be hidden in other
  // circumstances (e.g. while showing the native Windows WebAuthn UI). But in
  // those cases the UI is still enabled and can be shown e.g. for an
  // attestation consent prompt.
  return !disable_ui_;
}

#if defined(OS_MAC)
base::Optional<ChromeAuthenticatorRequestDelegate::TouchIdAuthenticatorConfig>
ChromeAuthenticatorRequestDelegate::GetTouchIdAuthenticatorConfig() {
  return TouchIdAuthenticatorConfigForProfile(
      electron::ElectronBrowserContext::FromBrowserContext(browser_context()));
}
#endif  // defined(OS_MAC)

void ChromeAuthenticatorRequestDelegate::OnTransportAvailabilityEnumerated(
    device::FidoRequestHandlerBase::TransportAvailabilityInfo data) {
  if (disable_ui_ || !transient_dialog_model_holder_) {
    return;
  }

  weak_dialog_model_->AddObserver(this);

  weak_dialog_model_->StartFlow(std::move(data), GetLastTransportUsed());

  ShowAuthenticatorRequestDialog(
      content::WebContents::FromRenderFrameHost(render_frame_host()),
      std::move(transient_dialog_model_holder_));
}

bool ChromeAuthenticatorRequestDelegate::EmbedderControlsAuthenticatorDispatch(
    const device::FidoAuthenticator& authenticator) {
  // Decide whether the //device/fido code should dispatch the current
  // request to an authenticator immediately after it has been
  // discovered, or whether the embedder/UI takes charge of that by
  // invoking its RequestCallback.
  auto transport = authenticator.AuthenticatorTransport();
  return IsWebAuthnUIEnabled() &&
         (!transport ||  // Windows
          *transport == device::FidoTransportProtocol::kInternal);
}

void ChromeAuthenticatorRequestDelegate::FidoAuthenticatorAdded(
    const device::FidoAuthenticator& authenticator) {
  if (!IsWebAuthnUIEnabled())
    return;

  if (!weak_dialog_model_)
    return;

  weak_dialog_model_->AddAuthenticator(authenticator);
}

void ChromeAuthenticatorRequestDelegate::FidoAuthenticatorRemoved(
    base::StringPiece authenticator_id) {
  if (!IsWebAuthnUIEnabled())
    return;

  if (!weak_dialog_model_)
    return;

  weak_dialog_model_->RemoveAuthenticator(authenticator_id);
}

void ChromeAuthenticatorRequestDelegate::BluetoothAdapterPowerChanged(
    bool is_powered_on) {
  if (!weak_dialog_model_)
    return;

  weak_dialog_model_->OnBluetoothPoweredStateChanged(is_powered_on);
}

bool ChromeAuthenticatorRequestDelegate::SupportsPIN() const {
  return true;
}

void ChromeAuthenticatorRequestDelegate::CollectPIN(
    CollectPINOptions options,
    base::OnceCallback<void(base::string16)> provide_pin_cb) {
  if (!weak_dialog_model_)
    return;

  weak_dialog_model_->CollectPIN(options.reason, options.error,
                                 options.min_pin_length, options.attempts,
                                 std::move(provide_pin_cb));
}

void ChromeAuthenticatorRequestDelegate::StartBioEnrollment(
    base::OnceClosure next_callback) {
  if (!weak_dialog_model_)
    return;

  weak_dialog_model_->StartInlineBioEnrollment(std::move(next_callback));
}

void ChromeAuthenticatorRequestDelegate::OnSampleCollected(
    int bio_samples_remaining) {
  if (!weak_dialog_model_)
    return;

  weak_dialog_model_->OnSampleCollected(bio_samples_remaining);
}

void ChromeAuthenticatorRequestDelegate::FinishCollectToken() {
  if (!weak_dialog_model_)
    return;

  weak_dialog_model_->SetCurrentStep(
      AuthenticatorRequestDialogModel::Step::kClientPinTapAgain);
}

void ChromeAuthenticatorRequestDelegate::OnRetryUserVerification(int attempts) {
  if (!weak_dialog_model_)
    return;

  weak_dialog_model_->OnRetryUserVerification(attempts);
}

void ChromeAuthenticatorRequestDelegate::SetMightCreateResidentCredential(
    bool v) {
  if (!weak_dialog_model_) {
    return;
  }
  weak_dialog_model_->set_might_create_resident_credential(v);
}

void ChromeAuthenticatorRequestDelegate::OnStartOver() {
  DCHECK(start_over_callback_);
  start_over_callback_.Run();
}

void ChromeAuthenticatorRequestDelegate::OnModelDestroyed() {
  DCHECK(weak_dialog_model_);
  weak_dialog_model_ = nullptr;
}

void ChromeAuthenticatorRequestDelegate::OnCancelRequest() {
  // |cancel_callback_| must be invoked at most once as invocation of
  // |cancel_callback_| will destroy |this|.
  DCHECK(cancel_callback_);
  std::move(cancel_callback_).Run();
}

base::Optional<device::FidoTransportProtocol>
ChromeAuthenticatorRequestDelegate::GetLastTransportUsed() const {
  PrefService* prefs =
      electron::ElectronBrowserContext::FromBrowserContext(browser_context())
          ->prefs();
  return device::ConvertToFidoTransportProtocol(
      prefs->GetString(kWebAuthnLastTransportUsedPrefName));
}

bool ChromeAuthenticatorRequestDelegate::ShouldPermitCableExtension(
    const url::Origin& origin) {
  if (base::FeatureList::IsEnabled(device::kWebAuthCableExtensionAnywhere)) {
    return true;
  }

  // Because the future of the caBLE extension might be that we transition
  // everything to QR-code or sync-based pairing, we don't want use of the
  // extension to spread without consideration. Therefore it's limited to
  // origins that are already depending on it and test sites.
  if (origin.DomainIs("google.com")) {
    return true;
  }

  const GURL test_site("https://webauthndemo.appspot.com");
  DCHECK(test_site.is_valid());
  return origin.IsSameOriginWith(url::Origin::Create(test_site));
}

std::vector<std::unique_ptr<device::cablev2::Pairing>>
ChromeAuthenticatorRequestDelegate::GetCablePairings() {
  std::vector<std::unique_ptr<device::cablev2::Pairing>> ret;
  if (!base::FeatureList::IsEnabled(device::kWebAuthPhoneSupport)) {
    NOTREACHED();
    return ret;
  }

  PrefService* prefs =
      electron::ElectronBrowserContext::FromBrowserContext(browser_context())
          ->prefs();
  const base::ListValue* pref_pairings =
      prefs->GetList(kWebAuthnCablePairingsPrefName);

  for (const auto& pairing : *pref_pairings) {
    if (!pairing.is_dict()) {
      continue;
    }

    auto out_pairing = std::make_unique<device::cablev2::Pairing>();
    if (!CopyString(&out_pairing->name, GetString(pairing, kPairingPrefName)) ||
        !CopyString(&out_pairing->tunnel_server_domain,
                    GetString(pairing, kPairingPrefTunnelServer)) ||
        !CopyBytestring(&out_pairing->contact_id,
                        GetString(pairing, kPairingPrefContactId)) ||
        !CopyBytestring(&out_pairing->id, GetString(pairing, kPairingPrefId)) ||
        !CopyBytestring(&out_pairing->secret,
                        GetString(pairing, kPairingPrefSecret)) ||
        !CopyBytestring(&out_pairing->peer_public_key_x962,
                        GetString(pairing, kPairingPrefPublicKey))) {
      continue;
    }

    ret.emplace_back(std::move(out_pairing));
  }

  return ret;
}

void ChromeAuthenticatorRequestDelegate::HandleCablePairingEvent(
    device::cablev2::PairingEvent event) {
  // This is called when doing a QR-code pairing with a phone and the phone
  // sends long-term pairing information during the handshake. The pairing
  // information is saved in preferences for future operations.
  if (!base::FeatureList::IsEnabled(device::kWebAuthPhoneSupport)) {
    NOTREACHED();
    return;
  }

  // For Incognito/Guest profiles, pairings will only last for the duration of
  // that session. While an argument could be made that it's safe to persist
  // such pairing for longer, this seems like the safe option initially.
  ListPrefUpdate update(
      electron::ElectronBrowserContext::FromBrowserContext(browser_context())
          ->prefs(),
      kWebAuthnCablePairingsPrefName);

  if (auto* disabled_public_key =
          absl::get_if<std::array<uint8_t, device::kP256X962Length>>(&event)) {
    // A pairing was reported to be invalid. Delete it.
    DeleteCablePairingByPublicKey(update.Get(), Base64(*disabled_public_key));
    return;
  }

  // Otherwise the event is a new pairing.
  auto& pairing =
      *absl::get_if<std::unique_ptr<device::cablev2::Pairing>>(&event);
  // Find any existing entries with the same public key and replace them. The
  // handshake protocol requires the phone to prove possession of the public key
  // so it's not possible for an evil phone to displace another's pairing.
  std::string public_key_base64 = Base64(pairing->peer_public_key_x962);
  DeleteCablePairingByPublicKey(update.Get(), public_key_base64);

  auto dict = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  dict->SetKey(kPairingPrefPublicKey,
               base::Value(std::move(public_key_base64)));
  dict->SetKey(kPairingPrefTunnelServer,
               base::Value(pairing->tunnel_server_domain));
  dict->SetKey(kPairingPrefName, base::Value(std::move(pairing->name)));
  dict->SetKey(kPairingPrefContactId, base::Value(Base64(pairing->contact_id)));
  dict->SetKey(kPairingPrefId, base::Value(Base64(pairing->id)));
  dict->SetKey(kPairingPrefSecret, base::Value(Base64(pairing->secret)));

  base::Time::Exploded now;
  base::Time::Now().UTCExplode(&now);
  dict->SetKey(kPairingPrefTime,
               // RFC 3339 time format.
               base::Value(base::StringPrintf(
                   "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year, now.month,
                   now.day_of_month, now.hour, now.minute, now.second)));

  update->Append(std::move(dict));
}
