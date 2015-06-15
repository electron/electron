// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_cookies.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/bind.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/callback.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_util.h"
#include "net/url_request/url_request_context.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;

namespace {

bool GetCookieListFromStore(net::CookieStore* cookie_store,
    const std::string& url,
    const net::CookieMonster::GetCookieListCallback& callback) {
  DCHECK(cookie_store);
  GURL gurl(url);
  net::CookieMonster* monster = cookie_store->GetCookieMonster();
  // Empty url will match all url cookies.
  if (url.empty()) {
    monster->GetAllCookiesAsync(callback);
    return true;
  }

  if (!gurl.is_valid())
    return false;

  monster->GetAllCookiesForURLAsync(gurl, callback);
  return true;
}

void RunGetCookiesCallbackOnUIThread(const base::DictionaryValue* filter,
    const std::string& error_message, const net::CookieList& cookie_list,
    const atom::api::Cookies::CookiesCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Should release filter here.
  delete filter;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  if (!error_message.empty()) {
    v8::Local<v8::String> error = v8::String::NewFromUtf8(isolate,
        error_message.c_str());
    callback.Run(error, v8::Null(isolate));
    return;
  }
  callback.Run(v8::Null(isolate),
      mate::Converter<net::CookieList>::ToV8(isolate, cookie_list));
}

void RunRemoveCookiesCallbackOnUIThread(const std::string& error_message,
    const atom::api::Cookies::CookiesCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  if (!error_message.empty()) {
    v8::Local<v8::String> error = v8::String::NewFromUtf8(isolate,
        error_message.c_str());
    callback.Run(error, v8::Null(isolate));
    return;
  }

  callback.Run(v8::Null(isolate), v8::Null(isolate));
}

void RunSetCookiesCallbackOnUIThread(const base::DictionaryValue* details,
    const std::string& error_message, bool set_success,
    const atom::api::Cookies::CookiesCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  delete details;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  if (!error_message.empty()) {
    v8::Local<v8::String> error = v8::String::NewFromUtf8(isolate,
        error_message.c_str());
    callback.Run(error, v8::Null(isolate));
    return;
  }

  callback.Run(v8::Null(isolate), v8::Boolean::New(isolate, set_success));
}

bool MatchesDomain(const base::DictionaryValue* filter,
    const std::string& cookie_domain) {
  std::string filter_domain;
  if (!filter->GetString("domain", &filter_domain))
    return true;

  // Add a leading '.' character to the filter domain if it doesn't exist.
  if (net::cookie_util::DomainIsHostOnly(filter_domain))
    filter_domain.insert(0, ".");

  std::string sub_domain(cookie_domain);
  // Strip any leading '.' character from the input cookie domain.
  if (!net::cookie_util::DomainIsHostOnly(sub_domain))
    sub_domain = sub_domain.substr(1);

  // Now check whether the domain argument is a subdomain of the filter domain.
  for (sub_domain.insert(0, ".");
       sub_domain.length() >= filter_domain.length();) {
    if (sub_domain == filter_domain) {
      return true;
    }
    const size_t next_dot = sub_domain.find('.', 1);  // Skip over leading dot.
    sub_domain.erase(0, next_dot);
  }
  return false;
}

bool MatchesCookie(const base::DictionaryValue* filter,
    const net::CanonicalCookie& cookie) {
  std::string name, domain, path;
  bool is_secure, session;
  if (filter->GetString("name", &name) && name != cookie.Name())
    return false;
  if (filter->GetString("path", &path) && path != cookie.Path())
    return false;
  if (!MatchesDomain(filter, cookie.Domain()))
    return false;
  if (filter->GetBoolean("secure", &is_secure) &&
      is_secure != cookie.IsSecure())
    return false;
  if (filter->GetBoolean("session", &session) &&
      session != cookie.IsPersistent())
    return false;
  return true;
}

}  // namespace

namespace mate {

template<>
struct Converter<net::CanonicalCookie> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
      const net::CanonicalCookie& val) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("name", val.Name());
    dict.Set("value", val.Value());
    dict.Set("source", val.Source());
    dict.Set("domain", val.Domain());
    dict.Set("path", val.Path());
    dict.Set("secure", val.IsSecure());
    dict.Set("http_only", val.IsHttpOnly());
    dict.Set("session", val.IsPersistent());
    if (!val.IsPersistent())
      dict.Set("expirationDate", val.ExpiryDate().ToDoubleT());
    return dict.GetHandle();
  }
};

}

namespace atom {

namespace api {

Cookies::Cookies() {
}

Cookies::~Cookies() {
}

void Cookies::Get(const base::DictionaryValue& options,
                  const CookiesCallback& callback) {
  // The filter will be deleted manually after callback function finishes.
  base::DictionaryValue* filter = options.DeepCopyWithoutEmptyChildren();

  content::BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&Cookies::GetCookiesOnIOThread, base::Unretained(this),
        filter, callback));
}

void Cookies::GetCookiesOnIOThread(const base::DictionaryValue* filter,
                                   const CookiesCallback& callback) {
  net::CookieStore* cookie_store =
      AtomBrowserContext::Get()->url_request_context_getter()
      ->GetURLRequestContext()->cookie_store();
  std::string url;
  filter->GetString("url", &url);
  if (!GetCookieListFromStore(cookie_store, url,
          base::Bind(&Cookies::OnGetCookies, base::Unretained(this), filter,
              callback))) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&RunGetCookiesCallbackOnUIThread, filter, "Url is not valid",
        net::CookieList(), callback));
  }
}

void Cookies::OnGetCookies(const base::DictionaryValue* filter,
                           const CookiesCallback& callback,
                           const net::CookieList& cookie_list) {
  net::CookieList result;
  for (const auto& cookie : cookie_list) {
    if (MatchesCookie(filter, cookie))
      result.push_back(cookie);
  }
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &RunGetCookiesCallbackOnUIThread, filter, "", result, callback));
}

void Cookies::Remove(const base::DictionaryValue& details,
                     const CookiesCallback& callback) {
  std::string url, name;
  std::string error_message;
  if (!details.GetString("url", &url) || !details.GetString("name", &name)) {
    error_message = "Details(url, name) of removing cookie are required.";
  }
  GURL gurl(url);
  if (error_message.empty() && !gurl.is_valid()) {
    error_message = "Url is not valid.";
  }
  if (!error_message.empty()) {
     RunRemoveCookiesCallbackOnUIThread(error_message, callback);
     return;
  }
  content::BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&Cookies::RemoveCookiesOnIOThread, base::Unretained(this),
        gurl, name, callback));
}

void Cookies::RemoveCookiesOnIOThread(const GURL& url, const std::string& name,
    const CookiesCallback& callback) {
  net::CookieStore* cookie_store =
      AtomBrowserContext::Get()->url_request_context_getter()
      ->GetURLRequestContext()->cookie_store();
  cookie_store->DeleteCookieAsync(url, name,
      base::Bind(&Cookies::OnRemoveCookies, base::Unretained(this), callback));
}

void Cookies::OnRemoveCookies(const CookiesCallback& callback) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&RunRemoveCookiesCallbackOnUIThread, "", callback));
}

void Cookies::Set(const base::DictionaryValue& options,
                  const CookiesCallback& callback) {
  std::string url;
  std::string error_message;
  if (!options.GetString("url", &url)) {
    error_message = "The url field is required.";
  }

  GURL gurl(url);
  if (error_message.empty() && !gurl.is_valid()) {
    error_message = "Url is not valid.";
  }

  if (!error_message.empty()) {
     RunSetCookiesCallbackOnUIThread(nullptr, error_message, false, callback);
     return;
  }

  // The filter will be deleted manually after callback function finishes.
  base::DictionaryValue* details = options.DeepCopyWithoutEmptyChildren();

  content::BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&Cookies::SetCookiesOnIOThread, base::Unretained(this),
        details, gurl, callback));
}

void Cookies::SetCookiesOnIOThread(const base::DictionaryValue* details,
                                   const GURL& url,
                                   const CookiesCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  net::CookieStore* cookie_store =
      AtomBrowserContext::Get()->url_request_context_getter()
      ->GetURLRequestContext()->cookie_store();

  std::string name, value, domain, path;
  bool secure = false;
  bool http_only = false;
  double expiration_date;

  details->GetString("name", &name);
  details->GetString("value", &value);
  details->GetString("domain", &domain);
  details->GetString("path", &path);
  details->GetBoolean("secure", &secure);
  details->GetBoolean("http_only", &http_only);

  base::Time expiration_time;
  if (details->GetDouble("expirationDate", &expiration_date)) {
    expiration_time = (expiration_date == 0) ?
        base::Time::UnixEpoch() :
        base::Time::FromDoubleT(expiration_date);
  }

  cookie_store->GetCookieMonster()->SetCookieWithDetailsAsync(
      url,
      name,
      value,
      domain,
      path,
      expiration_time,
      secure,
      http_only,
      false,
      net::COOKIE_PRIORITY_DEFAULT,
      base::Bind(&Cookies::OnSetCookies, base::Unretained(this), details,
          callback));
}

void Cookies::OnSetCookies(const base::DictionaryValue* details,
                           const CookiesCallback& callback,
                           bool set_success) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&RunSetCookiesCallbackOnUIThread, details, "", set_success,
          callback));
}

mate::ObjectTemplateBuilder Cookies::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("get", &Cookies::Get)
      .SetMethod("remove", &Cookies::Remove)
      .SetMethod("set", &Cookies::Set);
}

// static
mate::Handle<Cookies> Cookies::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new Cookies);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("cookies", atom::api::Cookies::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_cookies, Initialize);
