// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_COOKIES_H_
#define ATOM_BROWSER_API_ATOM_API_COOKIES_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/net/cookie_details.h"
#include "base/callback_list.h"
#include "native_mate/handle.h"
#include "net/cookies/canonical_cookie.h"

namespace base {
class DictionaryValue;
}

namespace net {
class URLRequestContextGetter;
}

namespace atom {

class AtomBrowserContext;

namespace api {

class Cookies : public mate::TrackableObject<Cookies> {
 public:
  enum Error {
    SUCCESS,
    FAILED,
  };

  using GetCallback = base::Callback<void(Error, const net::CookieList&)>;
  using SetCallback = base::Callback<void(Error)>;

  static mate::Handle<Cookies> Create(v8::Isolate* isolate,
                                      AtomBrowserContext* browser_context);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  Cookies(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~Cookies() override;

  void Get(const base::DictionaryValue& filter, const GetCallback& callback);
  void Remove(const GURL& url,
              const std::string& name,
              const base::Closure& callback);
  void Set(const base::DictionaryValue& details, const SetCallback& callback);
  void FlushStore(const base::Closure& callback);

  // CookieChangeNotifier subscription:
  void OnCookieChanged(const CookieDetails*);

 private:
  std::unique_ptr<base::CallbackList<void(const CookieDetails*)>::Subscription>
      cookie_change_subscription_;
  scoped_refptr<AtomBrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Cookies);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_COOKIES_H_
