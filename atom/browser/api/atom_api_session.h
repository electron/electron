// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SESSION_H_
#define ATOM_BROWSER_API_ATOM_API_SESSION_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "native_mate/handle.h"

class GURL;

namespace atom {

class AtomBrowserContext;

namespace api {

class Session: public mate::TrackableObject<Session> {
 public:
  using ResolveProxyCallback = base::Callback<void(std::string)>;

  // Gets or creates Session from the |browser_context|.
  static mate::Handle<Session> CreateFrom(
      v8::Isolate* isolate, AtomBrowserContext* browser_context);

 protected:
  explicit Session(AtomBrowserContext* browser_context);
  ~Session();

  // mate::Wrappable implementations:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  void ResolveProxy(const GURL& url, ResolveProxyCallback callback);
  v8::Local<v8::Value> Cookies(v8::Isolate* isolate);

  v8::Global<v8::Value> cookies_;

  AtomBrowserContext* browser_context_;  // weak ref

  DISALLOW_COPY_AND_ASSIGN(Session);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SESSION_H_
