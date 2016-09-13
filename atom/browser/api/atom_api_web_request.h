// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_REQUEST_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_REQUEST_H_

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "native_mate/arguments.h"
#include "native_mate/handle.h"

namespace atom {

class AtomBrowserContext;

namespace api {

class WebRequest : public mate::TrackableObject<WebRequest> {
 public:
  static mate::Handle<WebRequest> Create(v8::Isolate* isolate,
                                         AtomBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  WebRequest(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~WebRequest() override;

  // C++ can not distinguish overloaded member function.
  template<AtomNetworkDelegate::SimpleEvent type>
  void SetSimpleListener(mate::Arguments* args);
  template<AtomNetworkDelegate::ResponseEvent type>
  void SetResponseListener(mate::Arguments* args);
  template<typename Listener, typename Method, typename Event>
  void SetListener(Method method, Event type, mate::Arguments* args);

 private:
  scoped_refptr<AtomBrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(WebRequest);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_REQUEST_H_
