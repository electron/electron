// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/autofill/atom_autofill_client.h"

#include "atom/browser/autofill/personal_data_manager_factory.h"
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_context.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/origin_util.h"
#include "google_apis/gaia/identity_provider.h"
#include "native_mate/dictionary.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(autofill::AtomAutofillClient);

// stubs TODO - move to separate files
#include "net/http/http_request_headers.h"
namespace variations {
void AppendVariationHeaders(const GURL& url,
                            bool incognito,
                            bool uma_enabled,
                            net::HttpRequestHeaders* headers) {
}
}
#if !defined(OS_LINUX)
namespace rappor {
void SampleDomainAndRegistryFromGURL(RapporService* rappor_service,
                                     const std::string& metric,
                                     const GURL& gurl) {}
}  // namespace rappor
#endif
// end stubs

namespace mate {

template<>
struct Converter<autofill::Suggestion> {
  static v8::Local<v8::Value> ToV8(
    v8::Isolate* isolate, autofill::Suggestion val) {
    mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
    dict.Set("value", val.value);
    dict.Set("frontend_id", val.frontend_id);
    return dict.GetHandle();
  }
};
}  // namespace mate

class StubIdentityProvider : public IdentityProvider{
 public:
  StubIdentityProvider() {}
  ~StubIdentityProvider() override {}

  std::string GetActiveUsername() override {
    return std::string();
  };

  std::string GetActiveAccountId() override {
    return std::string();
  ;}

  OAuth2TokenService* GetTokenService() override {
    return nullptr;
  };

  bool RequestLogin() override {
    return false;
  };
};

namespace autofill {

AtomAutofillClient::AtomAutofillClient(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      api_web_contents_(nullptr) {
  DCHECK(web_contents);
}

AtomAutofillClient::~AtomAutofillClient() {
}

void AtomAutofillClient::Initialize(atom::api::WebContents* api_web_contents) {
  api_web_contents_ = api_web_contents;
}

void AtomAutofillClient::DidAcceptSuggestion(const std::string& value,
                                             int frontend_id,
                                             int index) {
  if (delegate_) {
    delegate_->DidAcceptSuggestion(base::UTF8ToUTF16(value), frontend_id,
                                   index);
  }
}

PersonalDataManager* AtomAutofillClient::GetPersonalDataManager() {
  content::BrowserContext* context = web_contents()->GetBrowserContext();
  return PersonalDataManagerFactory::GetForBrowserContext(context);
}

scoped_refptr<AutofillWebDataService> AtomAutofillClient::GetDatabase() {
  content::BrowserContext* context = web_contents()->GetBrowserContext();
  return static_cast<brave::BraveBrowserContext*>(context)
                ->GetAutofillWebdataService();
}

PrefService* AtomAutofillClient::GetPrefs() {
  return user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext());
}

sync_driver::SyncService* AtomAutofillClient::GetSyncService() {
  return nullptr;
}

IdentityProvider* AtomAutofillClient::GetIdentityProvider() {
  if (!identity_provider_) {
     identity_provider_.reset(new StubIdentityProvider());
  }
  return identity_provider_.get();
}

rappor::RapporService* AtomAutofillClient::GetRapporService() {
  return nullptr;
}

void AtomAutofillClient::ShowAutofillSettings() {
  if (api_web_contents_) {
    api_web_contents_->Emit("show-autofill-settings");
  }
}

void AtomAutofillClient::ShowUnmaskPrompt(
    const CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<CardUnmaskDelegate> delegate) {
}

void AtomAutofillClient::OnUnmaskVerificationResult(
    PaymentsRpcResult result) {
}

void AtomAutofillClient::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    const base::Closure& callback) {
}

void AtomAutofillClient::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    const base::Closure& callback) {
}

void AtomAutofillClient::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) {
}

bool AtomAutofillClient::HasCreditCardScanFeature() {
  return false;
}

void AtomAutofillClient::ScanCreditCard(
    const CreditCardScanCallback& callback) {
}

void AtomAutofillClient::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<autofill::Suggestion>& suggestions,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  delegate_ = delegate;
  if (api_web_contents_) {
    v8::Locker locker(api_web_contents_->isolate());
    v8::HandleScope handle_scope(api_web_contents_->isolate());
    mate::Dictionary rect =
      mate::Dictionary::CreateEmpty(api_web_contents_->isolate());
    gfx::Rect client_area = web_contents()->GetContainerBounds();
    rect.Set("x", element_bounds.x());
    rect.Set("y", element_bounds.y());
    rect.Set("width", element_bounds.width());
    rect.Set("height", element_bounds.height());
    rect.Set("clientWidth", client_area.width());
    rect.Set("clientHeight", client_area.height());

    api_web_contents_->Emit("show-autofill-popup",
                            suggestions,
                            rect);
  }
  delegate->OnPopupShown();
}

void AtomAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  if (api_web_contents_) {
    api_web_contents_->Emit("update-autofill-popup-data-list-values",
                            values, labels);
  }
}

void AtomAutofillClient::HideAutofillPopup() {
  // TODO(anthony): conflict with context menu
  // delegate_.reset();
  if (api_web_contents_) {
    api_web_contents_->Emit("hide-autofill-popup");
  }
}

bool AtomAutofillClient::IsAutocompleteEnabled() {
  // For browser, Autocomplete is always enabled as part of Autofill.
  return GetPrefs()->GetBoolean(prefs::kAutofillEnabled);
}

void AtomAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<autofill::FormStructure*>& forms) {
}

void AtomAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {
}

void AtomAutofillClient::OnFirstUserGestureObserved() {
  ContentAutofillDriverFactory* factory =
    ContentAutofillDriverFactory::FromWebContents(web_contents());
  DCHECK(factory);

  for (content::RenderFrameHost* frame : web_contents()->GetAllFrames()) {
    // No need to notify non-live frames.
    // And actually they have no corresponding drivers in the factory's map.
    if (!frame->IsRenderFrameLive())
      continue;
    ContentAutofillDriver* driver = factory->DriverForFrame(frame);
    DCHECK(driver);
    driver->NotifyFirstUserGestureObservedInTab();
  }
}

bool AtomAutofillClient::IsContextSecure(const GURL& form_origin) {
  return content::IsOriginSecure(form_origin);
}

}  // namespace autofill
