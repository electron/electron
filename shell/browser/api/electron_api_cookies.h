// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_COOKIES_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_COOKIES_H_

#include <string>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "gin/handle.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/trackable_object.h"

namespace gin_helper {
class Dictionary;
}

namespace electron {

class ElectronBrowserContext;

namespace api {

class Cookies : public gin::Wrappable<Cookies>,
                public gin_helper::EventEmitterMixin<Cookies> {
 public:
  static gin::Handle<Cookies> Create(v8::Isolate* isolate,
                                     ElectronBrowserContext* browser_context);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  Cookies(const Cookies&) = delete;
  Cookies& operator=(const Cookies&) = delete;

 protected:
  Cookies(v8::Isolate* isolate, ElectronBrowserContext* browser_context);
  ~Cookies() override;

  v8::Local<v8::Promise> Get(v8::Isolate*,
                             const gin_helper::Dictionary& filter);
  v8::Local<v8::Promise> Set(v8::Isolate*, base::Value::Dict details);
  v8::Local<v8::Promise> Remove(v8::Isolate*,
                                const GURL& url,
                                const std::string& name);
  v8::Local<v8::Promise> FlushStore(v8::Isolate*);

  // CookieChangeNotifier subscription:
  void OnCookieChanged(const net::CookieChangeInfo& change);

 private:
  base::CallbackListSubscription cookie_change_subscription_;

  // Weak reference; ElectronBrowserContext is guaranteed to outlive us.
  raw_ptr<ElectronBrowserContext> browser_context_;
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_COOKIES_H_
