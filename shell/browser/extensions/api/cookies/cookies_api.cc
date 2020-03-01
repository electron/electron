// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implements the Chrome Extensions Cookies API.

#include "shell/browser/extensions/api/cookies/cookies_api.h"

#include <memory>
#include <utility>
#include <vector>

using content::BrowserThread;

namespace extensions {

namespace {

bool ParseUrl(const Extension* extension,
              const std::string& url_string,
              GURL* url,
              bool check_host_permissions,
              std::string* error) {
  *url = GURL(url_string);
  if (!url->is_valid()) {
    *error = ErrorUtils::FormatErrorMessage(
        cookies_api_constants::kInvalidUrlError, url_string);
    return false;
  }
  // Check against host permissions if needed.
  if (check_host_permissions &&
      !extension->permissions_data()->HasHostPermission(*url)) {
    *error = ErrorUtils::FormatErrorMessage(
        cookies_api_constants::kNoHostPermissionsError, url->spec());
    return false;
  }
  return true;
}

network::mojom::CookieManager* ParseStoreCookieManager(
    content::BrowserContext* function_context,
    bool include_incognito,
    std::string* store_id,
    std::string* error) {
  Profile* function_profile = Profile::FromBrowserContext(function_context);
  Profile* store_profile = nullptr;
  if (!store_id->empty()) {
    store_profile = cookies_helpers::ChooseProfileFromStoreId(
        *store_id, function_profile, include_incognito);
    if (!store_profile) {
      *error = ErrorUtils::FormatErrorMessage(
          cookies_api_constants::kInvalidStoreIdError, *store_id);
      return nullptr;
    }
  } else {
    store_profile = function_profile;
    *store_id = cookies_helpers::GetStoreIdFromProfile(store_profile);
  }

  return content::BrowserContext::GetDefaultStoragePartition(store_profile)
      ->GetCookieManagerForBrowserProcess();
}

}  // namespace

CookiesEventRouter::CookieChangeListener::CookieChangeListener(
    CookiesEventRouter* router,
    bool otr)
    : router_(router), otr_(otr) {}
CookiesEventRouter::CookieChangeListener::~CookieChangeListener() = default;

void CookiesEventRouter::CookieChangeListener::OnCookieChange(
    const net::CookieChangeInfo& change) {
  router_->OnCookieChange(otr_, change);
}

CookiesEventRouter::CookiesEventRouter(content::BrowserContext* context)
    : profile_(Profile::FromBrowserContext(context)) {
  MaybeStartListening();
  BrowserList::AddObserver(this);
}

CookiesEventRouter::~CookiesEventRouter() {
  BrowserList::RemoveObserver(this);
}

void CookiesEventRouter::OnCookieChange(bool otr,
                                        const net::CookieChangeInfo& change) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::unique_ptr<base::ListValue> args(new base::ListValue());
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetBoolean(cookies_api_constants::kRemovedKey,
                   change.cause != net::CookieChangeCause::INSERTED);

  Profile* profile =
      otr ? profile_->GetOffTheRecordProfile() : profile_->GetOriginalProfile();
  api::cookies::Cookie cookie = cookies_helpers::CreateCookie(
      change.cookie, cookies_helpers::GetStoreIdFromProfile(profile));
  dict->Set(cookies_api_constants::kCookieKey, cookie.ToValue());

  // Map the internal cause to an external string.
  std::string cause_dict_entry;
  switch (change.cause) {
    // Report an inserted cookie as an "explicit" change cause. All other causes
    // only make sense for deletions.
    case net::CookieChangeCause::INSERTED:
    case net::CookieChangeCause::EXPLICIT:
      cause_dict_entry = cookies_api_constants::kExplicitChangeCause;
      break;

    case net::CookieChangeCause::OVERWRITE:
      cause_dict_entry = cookies_api_constants::kOverwriteChangeCause;
      break;

    case net::CookieChangeCause::EXPIRED:
      cause_dict_entry = cookies_api_constants::kExpiredChangeCause;
      break;

    case net::CookieChangeCause::EVICTED:
      cause_dict_entry = cookies_api_constants::kEvictedChangeCause;
      break;

    case net::CookieChangeCause::EXPIRED_OVERWRITE:
      cause_dict_entry = cookies_api_constants::kExpiredOverwriteChangeCause;
      break;

    case net::CookieChangeCause::UNKNOWN_DELETION:
      NOTREACHED();
  }
  dict->SetString(cookies_api_constants::kCauseKey, cause_dict_entry);

  args->Append(std::move(dict));

  DispatchEvent(profile, events::COOKIES_ON_CHANGED,
                api::cookies::OnChanged::kEventName, std::move(args),
                cookies_helpers::GetURLFromCanonicalCookie(change.cookie));
}

void CookiesEventRouter::OnBrowserAdded(Browser* browser) {
  // The new browser may be associated with a profile that is the OTR spinoff
  // of |profile_|, in which case we need to start listening to cookie changes
  // there. If this is any other kind of new browser, MaybeStartListening() will
  // be a no op.
  MaybeStartListening();
}

void CookiesEventRouter::MaybeStartListening() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(profile_);

  Profile* original_profile = profile_->GetOriginalProfile();
  Profile* otr_profile = original_profile->HasOffTheRecordProfile()
                             ? original_profile->GetOffTheRecordProfile()
                             : nullptr;

  if (!receiver_.is_bound())
    BindToCookieManager(&receiver_, original_profile);
  if (!otr_receiver_.is_bound() && otr_profile)
    BindToCookieManager(&otr_receiver_, otr_profile);
}

void CookiesEventRouter::BindToCookieManager(
    mojo::Receiver<network::mojom::CookieChangeListener>* receiver,
    Profile* profile) {
  network::mojom::CookieManager* cookie_manager =
      content::BrowserContext::GetDefaultStoragePartition(profile)
          ->GetCookieManagerForBrowserProcess();
  if (!cookie_manager)
    return;

  cookie_manager->AddGlobalChangeListener(receiver->BindNewPipeAndPassRemote());
  receiver->set_disconnect_handler(
      base::BindOnce(&CookiesEventRouter::OnConnectionError,
                     base::Unretained(this), receiver));
}

void CookiesEventRouter::OnConnectionError(
    mojo::Receiver<network::mojom::CookieChangeListener>* receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  receiver->reset();
  MaybeStartListening();
}

void CookiesEventRouter::DispatchEvent(
    content::BrowserContext* context,
    events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> event_args,
    const GURL& cookie_domain) {
  EventRouter* router = context ? EventRouter::Get(context) : NULL;
  if (!router)
    return;
  auto event = std::make_unique<Event>(histogram_value, event_name,
                                       std::move(event_args), context);
  event->event_url = cookie_domain;
  router->BroadcastEvent(std::move(event));
}

CookiesGetFunction::CookiesGetFunction() = default;
CookiesGetFunction::~CookiesGetFunction() = default;

ExtensionFunction::ResponseAction CookiesGetFunction::Run() {
  parsed_args_ = api::cookies::Get::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(parsed_args_.get());

  // Read/validate input parameters.
  std::string error;
  if (!ParseUrl(extension(), parsed_args_->details.url, &url_, true, &error))
    return RespondNow(Error(error));

  std::string store_id = parsed_args_->details.store_id.get()
                             ? *parsed_args_->details.store_id
                             : std::string();
  network::mojom::CookieManager* cookie_manager = ParseStoreCookieManager(
      browser_context(), include_incognito_information(), &store_id, &error);
  if (!cookie_manager)
    return RespondNow(Error(error));

  if (!parsed_args_->details.store_id.get())
    parsed_args_->details.store_id.reset(new std::string(store_id));

  DCHECK(!url_.is_empty() && url_.is_valid());
  cookies_helpers::GetCookieListFromManager(
      cookie_manager, url_,
      base::BindOnce(&CookiesGetFunction::GetCookieListCallback, this));

  // Will finish asynchronously.
  return RespondLater();
}

void CookiesGetFunction::GetCookieListCallback(
    const net::CookieStatusList& cookie_status_list,
    const net::CookieStatusList& excluded_cookies) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (const net::CookieWithStatus& cookie_with_status : cookie_status_list) {
    // Return the first matching cookie. Relies on the fact that the
    // CookieManager interface returns them in canonical order (longest path,
    // then earliest creation time).
    if (cookie_with_status.cookie.Name() == parsed_args_->details.name) {
      api::cookies::Cookie api_cookie = cookies_helpers::CreateCookie(
          cookie_with_status.cookie, *parsed_args_->details.store_id);
      Respond(ArgumentList(api::cookies::Get::Results::Create(api_cookie)));
      return;
    }
  }

  // The cookie doesn't exist; return null.
  Respond(OneArgument(std::make_unique<base::Value>()));
}

CookiesGetAllFunction::CookiesGetAllFunction() {}

CookiesGetAllFunction::~CookiesGetAllFunction() {}

ExtensionFunction::ResponseAction CookiesGetAllFunction::Run() {
  parsed_args_ = api::cookies::GetAll::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(parsed_args_.get());

  std::string error;
  if (parsed_args_->details.url.get() &&
      !ParseUrl(extension(), *parsed_args_->details.url, &url_, false,
                &error)) {
    return RespondNow(Error(error));
  }

  std::string store_id = parsed_args_->details.store_id.get()
                             ? *parsed_args_->details.store_id
                             : std::string();
  network::mojom::CookieManager* cookie_manager = ParseStoreCookieManager(
      browser_context(), include_incognito_information(), &store_id, &error);
  if (!cookie_manager)
    return RespondNow(Error(error));

  if (!parsed_args_->details.store_id.get())
    parsed_args_->details.store_id.reset(new std::string(store_id));

  DCHECK(url_.is_empty() || url_.is_valid());
  if (url_.is_empty()) {
    cookies_helpers::GetAllCookiesFromManager(
        cookie_manager,
        base::BindOnce(&CookiesGetAllFunction::GetAllCookiesCallback, this));
  } else {
    cookies_helpers::GetCookieListFromManager(
        cookie_manager, url_,
        base::BindOnce(&CookiesGetAllFunction::GetCookieListCallback, this));
  }

  return RespondLater();
}

void CookiesGetAllFunction::GetAllCookiesCallback(
    const net::CookieList& cookie_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ResponseValue response;
  if (extension()) {
    std::vector<api::cookies::Cookie> match_vector;
    cookies_helpers::AppendMatchingCookiesFromCookieListToVector(
        cookie_list, &parsed_args_->details, extension(), &match_vector);

    response =
        ArgumentList(api::cookies::GetAll::Results::Create(match_vector));
  } else {
    // TODO(devlin): When can |extension()| be null for this function?
    response = NoArguments();
  }
  Respond(std::move(response));
}

void CookiesGetAllFunction::GetCookieListCallback(
    const net::CookieStatusList& cookie_status_list,
    const net::CookieStatusList& excluded_cookies) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ResponseValue response;
  if (extension()) {
    std::vector<api::cookies::Cookie> match_vector;
    cookies_helpers::AppendMatchingCookiesFromCookieStatusListToVector(
        cookie_status_list, &parsed_args_->details, extension(), &match_vector);

    response =
        ArgumentList(api::cookies::GetAll::Results::Create(match_vector));
  } else {
    // TODO(devlin): When can |extension()| be null for this function?
    response = NoArguments();
  }
  Respond(std::move(response));
}

CookiesSetFunction::CookiesSetFunction()
    : state_(NO_RESPONSE), success_(false) {}

CookiesSetFunction::~CookiesSetFunction() {}

ExtensionFunction::ResponseAction CookiesSetFunction::Run() {
  parsed_args_ = api::cookies::Set::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(parsed_args_.get());

  // Read/validate input parameters.
  std::string error;
  if (!ParseUrl(extension(), parsed_args_->details.url, &url_, true, &error))
    return RespondNow(Error(error));

  std::string store_id = parsed_args_->details.store_id.get()
                             ? *parsed_args_->details.store_id
                             : std::string();
  network::mojom::CookieManager* cookie_manager = ParseStoreCookieManager(
      browser_context(), include_incognito_information(), &store_id, &error);
  if (!cookie_manager)
    return RespondNow(Error(error));

  if (!parsed_args_->details.store_id.get())
    parsed_args_->details.store_id.reset(new std::string(store_id));

  base::Time expiration_time;
  if (parsed_args_->details.expiration_date.get()) {
    // Time::FromDoubleT converts double time 0 to empty Time object. So we need
    // to do special handling here.
    expiration_time =
        (*parsed_args_->details.expiration_date == 0)
            ? base::Time::UnixEpoch()
            : base::Time::FromDoubleT(*parsed_args_->details.expiration_date);
  }

  net::CookieSameSite same_site = net::CookieSameSite::UNSPECIFIED;
  switch (parsed_args_->details.same_site) {
    case api::cookies::SAME_SITE_STATUS_NO_RESTRICTION:
      same_site = net::CookieSameSite::NO_RESTRICTION;
      break;
    case api::cookies::SAME_SITE_STATUS_LAX:
      same_site = net::CookieSameSite::LAX_MODE;
      break;
    case api::cookies::SAME_SITE_STATUS_STRICT:
      same_site = net::CookieSameSite::STRICT_MODE;
      break;
    // This is the case if the optional sameSite property is given as
    // "unspecified":
    case api::cookies::SAME_SITE_STATUS_UNSPECIFIED:
    // This is the case if the optional sameSite property is left out:
    case api::cookies::SAME_SITE_STATUS_NONE:
      same_site = net::CookieSameSite::UNSPECIFIED;
      break;
  }

  // clang-format off
  std::unique_ptr<net::CanonicalCookie> cc(
      net::CanonicalCookie::CreateSanitizedCookie(
          url_, parsed_args_->details.name.get() ? *parsed_args_->details.name
                                                 : std::string(),
          parsed_args_->details.value.get() ? *parsed_args_->details.value
                                                 : std::string(),
          parsed_args_->details.domain.get() ? *parsed_args_->details.domain
                                             : std::string(),
          parsed_args_->details.path.get() ? *parsed_args_->details.path
                                           : std::string(),
          base::Time(),
          expiration_time,
          base::Time(),
          parsed_args_->details.secure.get() ? *parsed_args_->details.secure
                                             : false,
          parsed_args_->details.http_only.get() ?
              *parsed_args_->details.http_only :
              false,
          same_site,
          net::COOKIE_PRIORITY_DEFAULT));
  // clang-format on
  if (!cc) {
    // Return error through callbacks so that the proper error message
    // is generated.
    success_ = false;
    state_ = SET_COMPLETED;
    GetCookieListCallback(net::CookieStatusList(), net::CookieStatusList());
    return AlreadyResponded();
  }

  // Dispatch the setter, immediately followed by the getter.  This
  // plus FIFO ordering on the cookie_manager_ pipe means that no
  // other extension function will affect the get result.
  net::CookieOptions options;
  options.set_include_httponly();
  options.set_same_site_cookie_context(
      net::CookieOptions::SameSiteCookieContext::SAME_SITE_STRICT);
  DCHECK(!url_.is_empty() && url_.is_valid());
  cookie_manager->SetCanonicalCookie(
      *cc, url_.scheme(), options,
      base::BindOnce(&CookiesSetFunction::SetCanonicalCookieCallback, this));
  cookies_helpers::GetCookieListFromManager(
      cookie_manager, url_,
      base::BindOnce(&CookiesSetFunction::GetCookieListCallback, this));

  // Will finish asynchronously.
  return RespondLater();
}

void CookiesSetFunction::SetCanonicalCookieCallback(
    net::CanonicalCookie::CookieInclusionStatus set_cookie_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(NO_RESPONSE, state_);
  state_ = SET_COMPLETED;
  success_ = set_cookie_result.IsInclude();
}

void CookiesSetFunction::GetCookieListCallback(
    const net::CookieStatusList& cookie_list,
    const net::CookieStatusList& excluded_cookies) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(SET_COMPLETED, state_);
  state_ = GET_COMPLETED;

  if (!success_) {
    std::string name = parsed_args_->details.name.get()
                           ? *parsed_args_->details.name
                           : std::string();
    Respond(Error(ErrorUtils::FormatErrorMessage(
        cookies_api_constants::kCookieSetFailedError, name)));
    return;
  }

  ResponseValue value;
  for (const net::CookieWithStatus& cookie_with_status : cookie_list) {
    // Return the first matching cookie. Relies on the fact that the
    // CookieMonster returns them in canonical order (longest path, then
    // earliest creation time).
    std::string name = parsed_args_->details.name.get()
                           ? *parsed_args_->details.name
                           : std::string();
    if (cookie_with_status.cookie.Name() == name) {
      api::cookies::Cookie api_cookie = cookies_helpers::CreateCookie(
          cookie_with_status.cookie, *parsed_args_->details.store_id);
      value = ArgumentList(api::cookies::Set::Results::Create(api_cookie));
      break;
    }
  }

  Respond(value ? std::move(value) : NoArguments());
}

CookiesRemoveFunction::CookiesRemoveFunction() {}

CookiesRemoveFunction::~CookiesRemoveFunction() {}

ExtensionFunction::ResponseAction CookiesRemoveFunction::Run() {
  parsed_args_ = api::cookies::Remove::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(parsed_args_.get());

  // Read/validate input parameters.
  std::string error;
  if (!ParseUrl(extension(), parsed_args_->details.url, &url_, true, &error))
    return RespondNow(Error(error));

  std::string store_id = parsed_args_->details.store_id.get()
                             ? *parsed_args_->details.store_id
                             : std::string();
  network::mojom::CookieManager* cookie_manager = ParseStoreCookieManager(
      browser_context(), include_incognito_information(), &store_id, &error);
  if (!cookie_manager)
    return RespondNow(Error(error));

  if (!parsed_args_->details.store_id.get())
    parsed_args_->details.store_id.reset(new std::string(store_id));

  network::mojom::CookieDeletionFilterPtr filter(
      network::mojom::CookieDeletionFilter::New());
  filter->url = url_;
  filter->cookie_name = parsed_args_->details.name;
  cookie_manager->DeleteCookies(
      std::move(filter),
      base::BindOnce(&CookiesRemoveFunction::RemoveCookieCallback, this));

  // Will return asynchronously.
  return RespondLater();
}

void CookiesRemoveFunction::RemoveCookieCallback(uint32_t /* num_deleted */) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Build the callback result
  api::cookies::Remove::Results::Details details;
  details.name = parsed_args_->details.name;
  details.url = url_.spec();
  details.store_id = *parsed_args_->details.store_id;

  Respond(ArgumentList(api::cookies::Remove::Results::Create(details)));
}

ExtensionFunction::ResponseAction CookiesGetAllCookieStoresFunction::Run() {
  Profile* original_profile = Profile::FromBrowserContext(browser_context());
  DCHECK(original_profile);
  std::unique_ptr<base::ListValue> original_tab_ids(new base::ListValue());
  Profile* incognito_profile = NULL;
  std::unique_ptr<base::ListValue> incognito_tab_ids;
  if (include_incognito_information() &&
      original_profile->HasOffTheRecordProfile()) {
    incognito_profile = original_profile->GetOffTheRecordProfile();
    if (incognito_profile)
      incognito_tab_ids.reset(new base::ListValue());
  }
  DCHECK(original_profile != incognito_profile);

  // Iterate through all browser instances, and for each browser,
  // add its tab IDs to either the regular or incognito tab ID list depending
  // whether the browser is regular or incognito.
  for (auto* browser : *BrowserList::GetInstance()) {
    if (browser->profile() == original_profile) {
      cookies_helpers::AppendToTabIdList(browser, original_tab_ids.get());
    } else if (incognito_tab_ids.get() &&
               browser->profile() == incognito_profile) {
      cookies_helpers::AppendToTabIdList(browser, incognito_tab_ids.get());
    }
  }
  // Return a list of all cookie stores with at least one open tab.
  std::vector<api::cookies::CookieStore> cookie_stores;
  if (original_tab_ids->GetSize() > 0) {
    cookie_stores.push_back(cookies_helpers::CreateCookieStore(
        original_profile, std::move(original_tab_ids)));
  }
  if (incognito_tab_ids.get() && incognito_tab_ids->GetSize() > 0 &&
      incognito_profile) {
    cookie_stores.push_back(cookies_helpers::CreateCookieStore(
        incognito_profile, std::move(incognito_tab_ids)));
  }
  return RespondNow(ArgumentList(
      api::cookies::GetAllCookieStores::Results::Create(cookie_stores)));
}

CookiesAPI::CookiesAPI(content::BrowserContext* context)
    : browser_context_(context) {
  EventRouter::Get(browser_context_)
      ->RegisterObserver(this, api::cookies::OnChanged::kEventName);
}

CookiesAPI::~CookiesAPI() = default;

void CookiesAPI::Shutdown() {
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<CookiesAPI>>::
    DestructorAtExit g_cookies_api_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<CookiesAPI>* CookiesAPI::GetFactoryInstance() {
  return g_cookies_api_factory.Pointer();
}

void CookiesAPI::OnListenerAdded(const EventListenerInfo& details) {
  cookies_event_router_.reset(new CookiesEventRouter(browser_context_));
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

}  // namespace extensions