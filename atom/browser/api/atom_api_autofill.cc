// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_autofill.h"

#include "atom/browser/autofill/personal_data_manager_factory.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/brave_content_browser_client.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"

namespace mate {

template<>
struct Converter<autofill::AutofillProfile*> {
  static v8::Local<v8::Value> ToV8(
    v8::Isolate* isolate, autofill::AutofillProfile* val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  std::string full_name, company_name, street_address, city, state, locality,
    postal_code, sorting_code, country_code, phone, email;
  if (val) {
    full_name =
      base::UTF16ToUTF8(val->GetInfo(
        autofill::AutofillType(autofill::NAME_FULL),
        brave::BraveContentBrowserClient::Get()
        ->GetApplicationLocale()));
    if (!full_name.empty()) {
      dict.Set("full_name", full_name);
    }

    company_name =
      base::UTF16ToUTF8(val->GetRawInfo(autofill::COMPANY_NAME));
    if (!company_name.empty()) {
      dict.Set("company_name", company_name);
    }

    street_address =
      base::UTF16ToUTF8(val->GetRawInfo(
        autofill::ADDRESS_HOME_STREET_ADDRESS));
    if (!street_address.empty()) {
      dict.Set("street_address", street_address);
    }

    city =
      base::UTF16ToUTF8(val->GetRawInfo(
        autofill::ADDRESS_HOME_CITY));
    if (!city.empty()) {
      dict.Set("city", city);
    }

    state =
      base::UTF16ToUTF8(val->GetRawInfo(
        autofill::ADDRESS_HOME_STATE));
    if (!state.empty()) {
      dict.Set("state", state);
    }

    locality =
      base::UTF16ToUTF8(val->GetRawInfo(
        autofill::ADDRESS_HOME_DEPENDENT_LOCALITY));
    if (!locality.empty()) {
      dict.Set("locality", locality);
    }

    postal_code =
      base::UTF16ToUTF8(val->GetRawInfo(autofill::ADDRESS_HOME_ZIP));
    if (!postal_code.empty()) {
      dict.Set("postal_code", postal_code);
    }

    sorting_code =
      base::UTF16ToUTF8(val->GetRawInfo(
        autofill::ADDRESS_HOME_SORTING_CODE));
    if (!sorting_code.empty()) {
      dict.Set("sorting_code", sorting_code);
    }

    country_code =
      base::UTF16ToUTF8(val->GetRawInfo(
        autofill::ADDRESS_HOME_COUNTRY));
    if (!country_code.empty()) {
      dict.Set("country_code", country_code);
    }

    phone =
      base::UTF16ToUTF8(val->GetRawInfo(
        autofill::PHONE_HOME_WHOLE_NUMBER));
    if (!phone.empty()) {
      dict.Set("phone", phone);
    }

    email =
      base::UTF16ToUTF8(val->GetRawInfo(autofill::EMAIL_ADDRESS));
    if (!email.empty()) {
      dict.Set("email", email);
    }
  }
  return dict.GetHandle();
  }
};

template<>
struct Converter<autofill::CreditCard*> {
  static v8::Local<v8::Value> ToV8(
    v8::Isolate* isolate, autofill::CreditCard* val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  std::string name, card_number, expiration_month, expiration_year;
  if (val) {
    name =
    base::UTF16ToUTF8(val->GetRawInfo(autofill::CREDIT_CARD_NAME_FULL));
    if (!name.empty()) {
      dict.Set("name", name);
    }

    card_number =
      base::UTF16ToUTF8(val->GetRawInfo(autofill::CREDIT_CARD_NUMBER));
    if (!card_number.empty()) {
      dict.Set("card_number", card_number);
    }

    expiration_month =
      base::UTF16ToUTF8(val->GetRawInfo(autofill::CREDIT_CARD_EXP_MONTH));
    if (!expiration_month.empty()) {
      dict.Set("expiration_month", expiration_month);
    }

    expiration_year =
    base::UTF16ToUTF8(val->GetRawInfo(autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR));
    if (!expiration_year.empty()) {
      dict.Set("expiration_year", expiration_year);
    }
  }
  return dict.GetHandle();
  }
};

}  // namespace mate

namespace atom {

namespace api {

Autofill::Autofill(v8::Isolate* isolate,
                 content::BrowserContext* browser_context)
      : browser_context_(browser_context),
      weak_ptr_factory_(this) {
  Init(isolate);
}

Autofill::~Autofill() {
}

std::string Autofill::AddProfile(const base::DictionaryValue& profile) {
  std::string full_name, company_name, street_address, city, state, locality,
    postal_code, sorting_code, country_code, phone, email, language_code;
  profile.GetString("full_name", &full_name);
  profile.GetString("company_name", &company_name);
  profile.GetString("street_address", &street_address);
  profile.GetString("city", &city);
  profile.GetString("state", &state);
  profile.GetString("locality", &locality);
  profile.GetString("postal_code", &postal_code);
  profile.GetString("sorting_code", &sorting_code);
  profile.GetString("country_code", &country_code);
  profile.GetString("phone", &phone);
  profile.GetString("email", &email);
  profile.GetString("language_code", &language_code);
  autofill::PersonalDataManager* personal_data =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(
      browser_context_);
  if (!personal_data) {
    LOG(ERROR) << "No Data";
    return std::string();
  }

  autofill::AutofillProfile autofill_profile;
  if (!full_name.empty()) {
    autofill_profile.SetInfo(autofill::AutofillType(autofill::NAME_FULL),
                    base::UTF8ToUTF16(full_name),
                    brave::BraveContentBrowserClient::Get()
                    ->GetApplicationLocale());
  }

  if (!company_name.empty()) {
    autofill_profile.SetRawInfo(
        autofill::COMPANY_NAME,
        base::UTF8ToUTF16(company_name));
  }

  if (!street_address.empty()) {
    autofill_profile.SetRawInfo(
        autofill::ADDRESS_HOME_STREET_ADDRESS,
        base::UTF8ToUTF16(street_address));
  }

  if (!city.empty()) {
    autofill_profile.SetRawInfo(
        autofill::ADDRESS_HOME_CITY,
        base::UTF8ToUTF16(city));
  }

  if (!state.empty()) {
    autofill_profile.SetRawInfo(
        autofill::ADDRESS_HOME_STATE,
        base::UTF8ToUTF16(state));
  }

  if (!locality.empty()) {
    autofill_profile.SetRawInfo(
        autofill::ADDRESS_HOME_DEPENDENT_LOCALITY,
        base::UTF8ToUTF16(locality));
  }

  if (!postal_code.empty()) {
    autofill_profile.SetRawInfo(
        autofill::ADDRESS_HOME_ZIP,
        base::UTF8ToUTF16(postal_code));
  }

  if (!sorting_code.empty()) {
    autofill_profile.SetRawInfo(
        autofill::ADDRESS_HOME_SORTING_CODE,
        base::UTF8ToUTF16(sorting_code));
  }

  if (!country_code.empty()) {
    autofill_profile.SetRawInfo(
        autofill::ADDRESS_HOME_COUNTRY,
        base::UTF8ToUTF16(country_code));
  }

  if (!phone.empty()) {
    autofill_profile.SetRawInfo(autofill::PHONE_HOME_WHOLE_NUMBER,
        base::UTF8ToUTF16(phone));
  }

  if (!email.empty()) {
    autofill_profile.SetRawInfo(autofill::EMAIL_ADDRESS,
                                base::UTF8ToUTF16(email));
  }

  if (!language_code.empty())
    autofill_profile.set_language_code(language_code);

  return personal_data->SaveImportedProfile(autofill_profile);
}

autofill::AutofillProfile* Autofill::GetProfile(const std::string& guid) {
  autofill::PersonalDataManager* personal_data =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(
      browser_context_);
  if (!personal_data) {
    LOG(ERROR) << "No Data";
    return nullptr;
  }

  return personal_data->GetProfileByGUID(guid);
}

bool Autofill::RemoveProfile(const std::string& guid) {
  autofill::PersonalDataManager* personal_data =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(
      browser_context_);
  if (!personal_data) {
    LOG(ERROR) << "No Data";
    return false;
  }

  personal_data->RemoveByGUID(guid);
  return true;
}

std::string Autofill::AddCreditCard(const base::DictionaryValue& card) {
  std::string name, card_number, expiration_month, expiration_year;
  card.GetString("name", &name);
  card.GetString("card_number", &card_number);
  card.GetString("expiration_month", &expiration_month);
  card.GetString("expiration_year", &expiration_year);
  autofill::PersonalDataManager* personal_data =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(
      browser_context_);
  if (!personal_data) {
    LOG(ERROR) << "No Data";
    return std::string();
  }

  autofill::CreditCard credit_card;
  if (!name.empty()) {
    credit_card.SetRawInfo(autofill::CREDIT_CARD_NAME_FULL,
                           base::UTF8ToUTF16(name));
  }

  if (!card_number.empty()) {
    credit_card.SetRawInfo(
        autofill::CREDIT_CARD_NUMBER,
        base::UTF8ToUTF16(card_number));
  }

  if (!expiration_month.empty()) {
    credit_card.SetRawInfo(
        autofill::CREDIT_CARD_EXP_MONTH,
        base::UTF8ToUTF16(expiration_month));
  }

  if (!expiration_year.empty()) {
    credit_card.SetRawInfo(
        autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR,
        base::UTF8ToUTF16(expiration_year));
  }

  return personal_data->SaveImportedCreditCard(credit_card);
}

autofill::CreditCard* Autofill::GetCreditCard(const std::string& guid) {
  autofill::PersonalDataManager* personal_data =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(
      browser_context_);
  if (!personal_data) {
    return nullptr;
  }

  return personal_data->GetCreditCardByGUID(guid);
}

bool Autofill::RemoveCreditCard(const std::string& guid) {
  autofill::PersonalDataManager* personal_data =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(
      browser_context_);
  if (!personal_data) {
    return false;
  }

  personal_data->RemoveByGUID(guid);
  return true;
}

void Autofill::ClearAutocompleteData() {
  scoped_refptr<autofill::AutofillWebDataService> web_data_service =
    static_cast<brave::BraveBrowserContext*>(browser_context_)
    ->GetAutofillWebdataService();

  if (web_data_service.get()) {
    base::Time delete_begin;
    base::Time delete_end;
    // TODO(Anthony Tseng): Specify time range from front end
    delete_begin = delete_begin.UnixEpoch();
    delete_end = delete_end.Now();
    web_data_service->RemoveFormElementsAddedBetween(delete_begin,
                                                     delete_end);
    // thread. So wait for it.
    content::BrowserThread::PostTaskAndReply(content::BrowserThread::DB,
      FROM_HERE,
      base::Bind(&base::DoNothing),
      base::Bind(&Autofill::OnClearedAutocompleteData,
                 weak_ptr_factory_.GetWeakPtr()));

    autofill::PersonalDataManager* data_manager =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(
      browser_context_);
    if (data_manager)
      data_manager->Refresh();
  }
}

void Autofill::ClearAutofillData() {
  scoped_refptr<autofill::AutofillWebDataService> web_data_service =
    static_cast<brave::BraveBrowserContext*>(browser_context_)
    ->GetAutofillWebdataService();

  if (web_data_service.get()) {
    base::Time delete_begin;
    base::Time delete_end;
    // TODO(Anthony Tseng): Specify time range from front end
    delete_begin = delete_begin.UnixEpoch();
    delete_end = delete_end.Now();
    web_data_service->RemoveAutofillDataModifiedBetween(delete_begin,
                                                        delete_end);
    // The above calls are done on the UI thread but do their work on the DB
    // thread. So wait for it.
    content::BrowserThread::PostTaskAndReply(content::BrowserThread::DB,
      FROM_HERE,
      base::Bind(&base::DoNothing),
      base::Bind(&Autofill::OnClearedAutofillData,
                 weak_ptr_factory_.GetWeakPtr()));

    autofill::PersonalDataManager* data_manager =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(
      browser_context_);
    if (data_manager)
      data_manager->Refresh();
  }
}

void Autofill::OnClearedAutocompleteData() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void Autofill::OnClearedAutofillData() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

// static
mate::Handle<Autofill> Autofill::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new Autofill(isolate, browser_context));
}

// static
void Autofill::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Autofill"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
    .SetMethod("addProfile", &Autofill::AddProfile)
    .SetMethod("getProfile", &Autofill::GetProfile)
    .SetMethod("removeProfile", &Autofill::RemoveProfile)
    .SetMethod("addCreditCard", &Autofill::AddCreditCard)
    .SetMethod("getCreditCard", &Autofill::GetCreditCard)
    .SetMethod("removeCreditCard", &Autofill::RemoveCreditCard)
    .SetMethod("clearAutocompleteData", &Autofill::ClearAutocompleteData)
    .SetMethod("clearAutofillData", &Autofill::ClearAutofillData);
}

}  // namespace api

}  // namespace atom
