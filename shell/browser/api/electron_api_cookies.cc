// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_cookies.h"

#include <utility>

#include "base/time/time.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_util.h"
#include "shell/browser/cookie_change_notifier.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"

namespace gin {

template <>
struct Converter<net::CookieSameSite> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::CookieSameSite& val) {
    switch (val) {
      case net::CookieSameSite::UNSPECIFIED:
        return ConvertToV8(isolate, "unspecified");
      case net::CookieSameSite::NO_RESTRICTION:
        return ConvertToV8(isolate, "no_restriction");
      case net::CookieSameSite::LAX_MODE:
        return ConvertToV8(isolate, "lax");
      case net::CookieSameSite::STRICT_MODE:
        return ConvertToV8(isolate, "strict");
    }
    DCHECK(false);
    return ConvertToV8(isolate, "unknown");
  }
};

template <>
struct Converter<net::CanonicalCookie> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::CanonicalCookie& val) {
    gin::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("name", val.Name());
    dict.Set("value", val.Value());
    dict.Set("domain", val.Domain());
    dict.Set("hostOnly", net::cookie_util::DomainIsHostOnly(val.Domain()));
    dict.Set("path", val.Path());
    dict.Set("secure", val.IsSecure());
    dict.Set("httpOnly", val.IsHttpOnly());
    dict.Set("session", !val.IsPersistent());
    if (val.IsPersistent())
      dict.Set("expirationDate", val.ExpiryDate().ToDoubleT());
    dict.Set("sameSite", val.SameSite());
    return ConvertToV8(isolate, dict).As<v8::Object>();
  }
};

template <>
struct Converter<net::CookieChangeCause> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::CookieChangeCause& val) {
    switch (val) {
      case net::CookieChangeCause::INSERTED:
      case net::CookieChangeCause::EXPLICIT:
        return gin::StringToV8(isolate, "explicit");
      case net::CookieChangeCause::OVERWRITE:
        return gin::StringToV8(isolate, "overwrite");
      case net::CookieChangeCause::EXPIRED:
        return gin::StringToV8(isolate, "expired");
      case net::CookieChangeCause::EVICTED:
        return gin::StringToV8(isolate, "evicted");
      case net::CookieChangeCause::EXPIRED_OVERWRITE:
        return gin::StringToV8(isolate, "expired-overwrite");
      default:
        return gin::StringToV8(isolate, "unknown");
    }
  }
};

}  // namespace gin

namespace electron::api {

namespace {

// Returns whether |domain| matches |filter|.
bool MatchesDomain(std::string filter, const std::string& domain) {
  // Add a leading '.' character to the filter domain if it doesn't exist.
  if (net::cookie_util::DomainIsHostOnly(filter))
    filter.insert(0, ".");

  std::string sub_domain(domain);
  // Strip any leading '.' character from the input cookie domain.
  if (!net::cookie_util::DomainIsHostOnly(sub_domain))
    sub_domain = sub_domain.substr(1);

  // Now check whether the domain argument is a subdomain of the filter domain.
  for (sub_domain.insert(0, "."); sub_domain.length() >= filter.length();) {
    if (sub_domain == filter)
      return true;
    const size_t next_dot = sub_domain.find('.', 1);  // Skip over leading dot.
    sub_domain.erase(0, next_dot);
  }
  return false;
}

// Returns whether |cookie| matches |filter|.
bool MatchesCookie(const base::Value::Dict& filter,
                   const net::CanonicalCookie& cookie) {
  const std::string* str;
  if ((str = filter.FindString("name")) && *str != cookie.Name())
    return false;
  if ((str = filter.FindString("path")) && *str != cookie.Path())
    return false;
  if ((str = filter.FindString("domain")) &&
      !MatchesDomain(*str, cookie.Domain()))
    return false;
  absl::optional<bool> secure_filter = filter.FindBool("secure");
  if (secure_filter && *secure_filter == cookie.IsSecure())
    return false;
  absl::optional<bool> session_filter = filter.FindBool("session");
  if (session_filter && *session_filter != !cookie.IsPersistent())
    return false;
  return true;
}

// Remove cookies from |list| not matching |filter|, and pass it to |callback|.
void FilterCookies(base::Value::Dict filter,
                   gin_helper::Promise<net::CookieList> promise,
                   const net::CookieList& cookies) {
  net::CookieList result;
  for (const auto& cookie : cookies) {
    if (MatchesCookie(filter, cookie))
      result.push_back(cookie);
  }
  promise.Resolve(result);
}

void FilterCookieWithStatuses(
    base::Value::Dict filter,
    gin_helper::Promise<net::CookieList> promise,
    const net::CookieAccessResultList& list,
    const net::CookieAccessResultList& excluded_list) {
  FilterCookies(std::move(filter), std::move(promise),
                net::cookie_util::StripAccessResults(list));
}

// Parse dictionary property to CanonicalCookie time correctly.
base::Time ParseTimeProperty(const absl::optional<double>& value) {
  if (!value)  // empty time means ignoring the parameter
    return base::Time();
  if (*value == 0)  // FromDoubleT would convert 0 to empty Time
    return base::Time::UnixEpoch();
  return base::Time::FromDoubleT(*value);
}

std::string InclusionStatusToString(net::CookieInclusionStatus status) {
  if (status.HasExclusionReason(net::CookieInclusionStatus::EXCLUDE_HTTP_ONLY))
    return "Failed to create httponly cookie";
  if (status.HasExclusionReason(
          net::CookieInclusionStatus::EXCLUDE_SECURE_ONLY))
    return "Cannot create a secure cookie from an insecure URL";
  if (status.HasExclusionReason(
          net::CookieInclusionStatus::EXCLUDE_FAILURE_TO_STORE))
    return "Failed to parse cookie";
  if (status.HasExclusionReason(
          net::CookieInclusionStatus::EXCLUDE_INVALID_DOMAIN))
    return "Failed to get cookie domain";
  if (status.HasExclusionReason(
          net::CookieInclusionStatus::EXCLUDE_INVALID_PREFIX))
    return "Failed because the cookie violated prefix rules.";
  if (status.HasExclusionReason(
          net::CookieInclusionStatus::EXCLUDE_NONCOOKIEABLE_SCHEME))
    return "Cannot set cookie for current scheme";
  return "Setting cookie failed";
}

std::string StringToCookieSameSite(const std::string* str_ptr,
                                   net::CookieSameSite* same_site) {
  if (!str_ptr) {
    *same_site = net::CookieSameSite::LAX_MODE;
    return "";
  }
  const std::string& str = *str_ptr;
  if (str == "unspecified") {
    *same_site = net::CookieSameSite::UNSPECIFIED;
  } else if (str == "no_restriction") {
    *same_site = net::CookieSameSite::NO_RESTRICTION;
  } else if (str == "lax") {
    *same_site = net::CookieSameSite::LAX_MODE;
  } else if (str == "strict") {
    *same_site = net::CookieSameSite::STRICT_MODE;
  } else {
    return "Failed to convert '" + str +
           "' to an appropriate cookie same site value";
  }
  return "";
}

}  // namespace

gin::WrapperInfo Cookies::kWrapperInfo = {gin::kEmbedderNativeGin};

Cookies::Cookies(v8::Isolate* isolate, ElectronBrowserContext* browser_context)
    : browser_context_(browser_context) {
  cookie_change_subscription_ =
      browser_context_->cookie_change_notifier()->RegisterCookieChangeCallback(
          base::BindRepeating(&Cookies::OnCookieChanged,
                              base::Unretained(this)));
}

Cookies::~Cookies() = default;

v8::Local<v8::Promise> Cookies::Get(v8::Isolate* isolate,
                                    const gin_helper::Dictionary& filter) {
  gin_helper::Promise<net::CookieList> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* storage_partition = browser_context_->GetDefaultStoragePartition();
  auto* manager = storage_partition->GetCookieManagerForBrowserProcess();

  base::Value::Dict dict;
  gin::ConvertFromV8(isolate, filter.GetHandle(), &dict);

  std::string url;
  filter.Get("url", &url);
  if (url.empty()) {
    manager->GetAllCookies(
        base::BindOnce(&FilterCookies, std::move(dict), std::move(promise)));
  } else {
    net::CookieOptions options;
    options.set_include_httponly();
    options.set_same_site_cookie_context(
        net::CookieOptions::SameSiteCookieContext::MakeInclusive());
    options.set_do_not_update_access_time();

    manager->GetCookieList(GURL(url), options,
                           net::CookiePartitionKeyCollection::Todo(),
                           base::BindOnce(&FilterCookieWithStatuses,
                                          std::move(dict), std::move(promise)));
  }

  return handle;
}

v8::Local<v8::Promise> Cookies::Remove(v8::Isolate* isolate,
                                       const GURL& url,
                                       const std::string& name) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto cookie_deletion_filter = network::mojom::CookieDeletionFilter::New();
  cookie_deletion_filter->url = url;
  cookie_deletion_filter->cookie_name = name;

  auto* storage_partition = browser_context_->GetDefaultStoragePartition();
  auto* manager = storage_partition->GetCookieManagerForBrowserProcess();

  manager->DeleteCookies(
      std::move(cookie_deletion_filter),
      base::BindOnce(
          [](gin_helper::Promise<void> promise, uint32_t num_deleted) {
            gin_helper::Promise<void>::ResolvePromise(std::move(promise));
          },
          std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Cookies::Set(v8::Isolate* isolate,
                                    base::Value::Dict details) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  const std::string* url_string = details.FindString("url");
  if (!url_string) {
    promise.RejectWithErrorMessage("Missing required option 'url'");
    return handle;
  }
  const std::string* name = details.FindString("name");
  const std::string* value = details.FindString("value");
  const std::string* domain = details.FindString("domain");
  const std::string* path = details.FindString("path");
  bool http_only = details.FindBool("httpOnly").value_or(false);
  const std::string* same_site_string = details.FindString("sameSite");
  net::CookieSameSite same_site;
  std::string error = StringToCookieSameSite(same_site_string, &same_site);
  if (!error.empty()) {
    promise.RejectWithErrorMessage(error);
    return handle;
  }
  bool secure = details.FindBool("secure").value_or(
      same_site == net::CookieSameSite::NO_RESTRICTION);
  bool same_party =
      details.FindBool("sameParty")
          .value_or(secure && same_site != net::CookieSameSite::STRICT_MODE);

  GURL url(url_string ? *url_string : "");
  if (!url.is_valid()) {
    promise.RejectWithErrorMessage(
        InclusionStatusToString(net::CookieInclusionStatus(
            net::CookieInclusionStatus::EXCLUDE_INVALID_DOMAIN)));
    return handle;
  }

  auto canonical_cookie = net::CanonicalCookie::CreateSanitizedCookie(
      url, name ? *name : "", value ? *value : "", domain ? *domain : "",
      path ? *path : "", ParseTimeProperty(details.FindDouble("creationDate")),
      ParseTimeProperty(details.FindDouble("expirationDate")),
      ParseTimeProperty(details.FindDouble("lastAccessDate")), secure,
      http_only, same_site, net::COOKIE_PRIORITY_DEFAULT, same_party,
      absl::nullopt);
  if (!canonical_cookie || !canonical_cookie->IsCanonical()) {
    promise.RejectWithErrorMessage(
        InclusionStatusToString(net::CookieInclusionStatus(
            net::CookieInclusionStatus::EXCLUDE_FAILURE_TO_STORE)));
    return handle;
  }
  net::CookieOptions options;
  if (http_only) {
    options.set_include_httponly();
  }
  options.set_same_site_cookie_context(
      net::CookieOptions::SameSiteCookieContext::MakeInclusive());

  auto* storage_partition = browser_context_->GetDefaultStoragePartition();
  auto* manager = storage_partition->GetCookieManagerForBrowserProcess();
  manager->SetCanonicalCookie(
      *canonical_cookie, url, options,
      base::BindOnce(
          [](gin_helper::Promise<void> promise, net::CookieAccessResult r) {
            if (r.status.IsInclude()) {
              promise.Resolve();
            } else {
              promise.RejectWithErrorMessage(InclusionStatusToString(r.status));
            }
          },
          std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Cookies::FlushStore(v8::Isolate* isolate) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* storage_partition = browser_context_->GetDefaultStoragePartition();
  auto* manager = storage_partition->GetCookieManagerForBrowserProcess();

  manager->FlushCookieStore(base::BindOnce(
      gin_helper::Promise<void>::ResolvePromise, std::move(promise)));

  return handle;
}

void Cookies::OnCookieChanged(const net::CookieChangeInfo& change) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  Emit("changed", gin::ConvertToV8(isolate, change.cookie),
       gin::ConvertToV8(isolate, change.cause),
       gin::ConvertToV8(isolate,
                        change.cause != net::CookieChangeCause::INSERTED));
}

// static
gin::Handle<Cookies> Cookies::Create(v8::Isolate* isolate,
                                     ElectronBrowserContext* browser_context) {
  return gin::CreateHandle(isolate, new Cookies(isolate, browser_context));
}

gin::ObjectTemplateBuilder Cookies::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<Cookies>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("get", &Cookies::Get)
      .SetMethod("remove", &Cookies::Remove)
      .SetMethod("set", &Cookies::Set)
      .SetMethod("flushStore", &Cookies::FlushStore);
}

const char* Cookies::GetTypeName() {
  return "Cookies";
}

}  // namespace electron::api
