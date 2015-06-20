// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_COOKIES_H_
#define ATOM_BROWSER_API_ATOM_API_COOKIES_H_

#include <string>

#include "base/callback.h"
#include "base/values.h"
#include "native_mate/wrappable.h"
#include "native_mate/handle.h"
#include "native_mate/dictionary.h"

#include "net/cookies/canonical_cookie.h"

namespace content {
class BrowserContext;
}

namespace atom {

namespace api {

class Cookies : public mate::Wrappable {
 public:
  // node.js style callback function(error, result)
  typedef base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>
      CookiesCallback;

  static mate::Handle<Cookies> Create(v8::Isolate* isolate,
                                      content::BrowserContext* browser_context);

 protected:
  explicit Cookies(content::BrowserContext* browser_context);
  ~Cookies();

  void Get(const base::DictionaryValue& options,
           const CookiesCallback& callback);
  void Remove(const mate::Dictionary& details,
              const CookiesCallback& callback);
  void Set(const base::DictionaryValue& details,
           const CookiesCallback& callback);

  void GetCookiesOnIOThread(scoped_ptr<base::DictionaryValue> filter,
                            const CookiesCallback& callback);
  void OnGetCookies(scoped_ptr<base::DictionaryValue> filter,
                    const CookiesCallback& callback,
                    const net::CookieList& cookie_list);

  void RemoveCookiesOnIOThread(const GURL& url,
                               const std::string& name,
                               const CookiesCallback& callback);
  void OnRemoveCookies(const CookiesCallback& callback);

  void SetCookiesOnIOThread(scoped_ptr<base::DictionaryValue> details,
                            const GURL& url,
                            const CookiesCallback& callback);
  void OnSetCookies(const CookiesCallback& callback,
                    bool set_success);


  // mate::Wrappable implementations:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Cookies);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_COOKIES_H_
