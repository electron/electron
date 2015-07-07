// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
#define ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_

#include <string>
#include <map>
#include <vector>

#include "atom/browser/api/event_emitter.h"
#include "base/callback.h"
#include "native_mate/handle.h"
#include "net/base/completion_callback.h"

namespace net {
class URLRequest;
}

namespace atom {

class AtomBrowserContext;
class AtomURLRequestJobFactory;

namespace api {

class Protocol : public mate::EventEmitter {
 public:
  using JsProtocolHandler =
      base::Callback<v8::Local<v8::Value>(const net::URLRequest*)>;
  using JsCompletionCallback = base::Callback<void(v8::Local<v8::Value>)>;

  static mate::Handle<Protocol> Create(
      v8::Isolate* isolate, AtomBrowserContext* browser_context);

  JsProtocolHandler GetProtocolHandler(const std::string& scheme);

  AtomBrowserContext* browser_context() const { return browser_context_; }

 protected:
  explicit Protocol(AtomBrowserContext* browser_context);

  // mate::Wrappable implementations:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate);

 private:
  typedef std::map<std::string, JsProtocolHandler> ProtocolHandlersMap;

  // Callback called if protocol can be registered.
  void OnRegisterProtocol(const std::string& scheme,
                          const JsProtocolHandler& handler,
                          const JsCompletionCallback& callback,
                          int is_handled);
  // Callback called if protocol can be intercepted.
  void OnInterceptProtocol(const std::string& scheme,
                           const JsProtocolHandler& handler,
                           const JsCompletionCallback& callback,
                           int is_handled);

  // Register schemes to standard scheme list.
  void RegisterStandardSchemes(const std::vector<std::string>& schemes);

  // Register/unregister an networking |scheme| which would be handled by
  // |callback|.
  void RegisterProtocol(v8::Isolate* isolate,
                        const std::string& scheme,
                        const JsProtocolHandler& handler,
                        const JsCompletionCallback& callback);
  void UnregisterProtocol(v8::Isolate* isolate, const std::string& scheme,
                          const JsCompletionCallback& callback);

  // Returns whether a scheme has been registered.
  void IsHandledProtocol(const std::string& scheme,
                         const net::CompletionCallback& callback);

  // Intercept/unintercept an existing protocol handler.
  void InterceptProtocol(v8::Isolate* isolate,
                         const std::string& scheme,
                         const JsProtocolHandler& handler,
                         const JsCompletionCallback& callback);
  void UninterceptProtocol(v8::Isolate* isolate, const std::string& scheme,
                           const JsCompletionCallback& callback);

  // The networking related operations have to be done in IO thread.
  void RegisterProtocolInIO(const std::string& scheme);
  void UnregisterProtocolInIO(const std::string& scheme);
  void InterceptProtocolInIO(const std::string& scheme);
  void UninterceptProtocolInIO(const std::string& scheme);

  AtomBrowserContext* browser_context_;
  AtomURLRequestJobFactory* job_factory_;
  ProtocolHandlersMap protocol_handlers_;

  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
