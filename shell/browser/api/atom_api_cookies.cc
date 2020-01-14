// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_cookies.h"

#include <memory>
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
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_util.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/browser/cookie_change_notifier.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"

using content::BrowserThread;

namespace gin {

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

namespace electron {

namespace api {

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
bool MatchesCookie(const base::Value& filter,
                   const net::CanonicalCookie& cookie) {
  const std::string* str;
  if ((str = filter.FindStringKey("name")) && *str != cookie.Name())
    return false;
  if ((str = filter.FindStringKey("path")) && *str != cookie.Path())
    return false;
  if ((str = filter.FindStringKey("domain")) &&
      !MatchesDomain(*str, cookie.Domain()))
    return false;
  base::Optional<bool> secure_filter = filter.FindBoolKey("secure");
  if (secure_filter && *secure_filter == cookie.IsSecure())
    return false;
  base::Optional<bool> session_filter = filter.FindBoolKey("session");
  if (session_filter && *session_filter != !cookie.IsPersistent())
    return false;
  return true;
}

// Remove cookies from |list| not matching |filter|, and pass it to |callback|.
void FilterCookies(const base::Value& filter,
                   gin_helper::Promise<net::CookieList> promise,
                   const net::CookieList& cookies) {
  net::CookieList result;
  for (const auto& cookie : cookies) {
    if (MatchesCookie(filter, cookie))
      result.push_back(cookie);
  }
  promise.Resolve(result);
}

void FilterCookieWithStatuses(const base::Value& filter,
                              gin_helper::Promise<net::CookieList> promise,
                              const net::CookieStatusList& list,
                              const net::CookieStatusList& excluded_list) {
  FilterCookies(filter, std::move(promise),
                net::cookie_util::StripStatuses(list));
}

// Parse dictionary property to CanonicalCookie time correctly.
base::Time ParseTimeProperty(const base::Optional<double>& value) {
  if (!value)  // empty time means ignoring the parameter
    return base::Time();
  if (*value == 0)  // FromDoubleT would convert 0 to empty Time
    return base::Time::UnixEpoch();
  return base::Time::FromDoubleT(*value);
}

std::string InclusionStatusToString(
    net::CanonicalCookie::CookieInclusionStatus status) {
  if (status.HasExclusionReason(
          net::CanonicalCookie::CookieInclusionStatus::EXCLUDE_HTTP_ONLY))
    return "Failed to create httponly cookie";
  if (status.HasExclusionReason(
          net::CanonicalCookie::CookieInclusionStatus::EXCLUDE_SECURE_ONLY))
    return "Cannot create a secure cookie from an insecure URL";
  if (status.HasExclusionReason(net::CanonicalCookie::CookieInclusionStatus::
                                    EXCLUDE_FAILURE_TO_STORE))
    return "Failed to parse cookie";
  if (status.HasExclusionReason(
          net::CanonicalCookie::CookieInclusionStatus::EXCLUDE_INVALID_DOMAIN))
    return "Failed to get cookie domain";
  if (status.HasExclusionReason(
          net::CanonicalCookie::CookieInclusionStatus::EXCLUDE_INVALID_PREFIX))
    return "Failed because the cookie violated prefix rules.";
  if (status.HasExclusionReason(net::CanonicalCookie::CookieInclusionStatus::
                                    EXCLUDE_NONCOOKIEABLE_SCHEME))
    return "Cannot set cookie for current scheme";
  return "Setting cookie failed";
}

}  // namespace

Cookies::Cookies(v8::Isolate* isolate, AtomBrowserContext* browser_context)
    : browser_context_(browser_context) {
  Init(isolate);
  cookie_change_subscription_ =
      browser_context_->cookie_change_notifier()->RegisterCookieChangeCallback(
          base::BindRepeating(&Cookies::OnCookieChanged,
                              base::Unretained(this)));
}

Cookies::~Cookies() = default;

v8::Local<v8::Promise> Cookies::Get(const gin_helper::Dictionary& filter) {
  gin_helper::Promise<net::CookieList> promise(isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      browser_context_.get());
  auto* manager = storage_partition->GetCookieManagerForBrowserProcess();

  base::DictionaryValue dict;
  gin::ConvertFromV8(isolate(), filter.GetHandle(), &dict);

  std::string url;
  filter.Get("url", &url);
  if (url.empty()) {
    manager->GetAllCookies(
        base::BindOnce(&FilterCookies, std::move(dict), std::move(promise)));
  } else {
    net::CookieOptions options;
    options.set_include_httponly();
    options.set_same_site_cookie_context(
        net::CookieOptions::SameSiteCookieContext::SAME_SITE_STRICT);
    options.set_do_not_update_access_time();

    manager->GetCookieList(GURL(url), options,
                           base::BindOnce(&FilterCookieWithStatuses,
                                          std::move(dict), std::move(promise)));
  }

  return handle;
}

v8::Local<v8::Promise> Cookies::Remove(const GURL& url,
                                       const std::string& name) {
  gin_helper::Promise<void> promise(isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto cookie_deletion_filter = network::mojom::CookieDeletionFilter::New();
  cookie_deletion_filter->url = url;
  cookie_deletion_filter->cookie_name = name;

  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      browser_context_.get());
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

v8::Local<v8::Promise> Cookies::Set(base::DictionaryValue details) {
  gin_helper::Promise<void> promise(isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  const std::string* url_string = details.FindStringKey("url");
  const std::string* name = details.FindStringKey("name");
  const std::string* value = details.FindStringKey("value");
  const std::string* domain = details.FindStringKey("domain");
  const std::string* path = details.FindStringKey("path");
  bool secure = details.FindBoolKey("secure").value_or(false);
  bool http_only = details.FindBoolKey("httpOnly").value_or(false);

  GURL url(url_string ? *url_string : "");
  if (!url.is_valid()) {
    promise.RejectWithErrorMessage(
        InclusionStatusToString(net::CanonicalCookie::CookieInclusionStatus(
            net::CanonicalCookie::CookieInclusionStatus::
                EXCLUDE_INVALID_DOMAIN)));
    return handle;
  }

  auto canonical_cookie = net::CanonicalCookie::CreateSanitizedCookie(
      url, name ? *name : "", value ? *value : "", domain ? *domain : "",
      path ? *path : "",
      ParseTimeProperty(details.FindDoubleKey("creationDate")),
      ParseTimeProperty(details.FindDoubleKey("expirationDate")),
      ParseTimeProperty(details.FindDoubleKey("lastAccessDate")), secure,
      http_only, net::CookieSameSite::NO_RESTRICTION,
      net::COOKIE_PRIORITY_DEFAULT);
  if (!canonical_cookie || !canonical_cookie->IsCanonical()) {
    promise.RejectWithErrorMessage(
        InclusionStatusToString(net::CanonicalCookie::CookieInclusionStatus(
            net::CanonicalCookie::CookieInclusionStatus::
                EXCLUDE_FAILURE_TO_STORE)));
    return handle;
  }
  net::CookieOptions options;
  if (http_only) {
    options.set_include_httponly();
  }

  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      browser_context_.get());
  auto* manager = storage_partition->GetCookieManagerForBrowserProcess();
  manager->SetCanonicalCookie(
      *canonical_cookie, url.scheme(), options,
      base::BindOnce(
          [](gin_helper::Promise<void> promise,
             net::CanonicalCookie::CookieInclusionStatus status) {
            if (status.IsInclude()) {
              promise.Resolve();
            } else {
              promise.RejectWithErrorMessage(InclusionStatusToString(status));
            }
          },
          std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Cookies::FlushStore() {
  gin_helper::Promise<void> promise(isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      browser_context_.get());
  auto* manager = storage_partition->GetCookieManagerForBrowserProcess();

  manager->FlushCookieStore(base::BindOnce(
      gin_helper::Promise<void>::ResolvePromise, std::move(promise)));

  return handle;
}

void Cookies::OnCookieChanged(const net::CookieChangeInfo& change) {
  Emit("changed", gin::ConvertToV8(isolate(), change.cookie),
       gin::ConvertToV8(isolate(), change.cause),
       gin::ConvertToV8(isolate(),
                        change.cause != net::CookieChangeCause::INSERTED));
}

// static
gin::Handle<Cookies> Cookies::Create(v8::Isolate* isolate,
                                     AtomBrowserContext* browser_context) {
  return gin::CreateHandle(isolate, new Cookies(isolate, browser_context));
}

// static
void Cookies::BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "Cookies"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("get", &Cookies::Get)
      .SetMethod("remove", &Cookies::Remove)
      .SetMethod("set", &Cookies::Set)
      .SetMethod("flushStore", &Cookies::FlushStore);
}

}  // namespace api

}  // namespace electron
