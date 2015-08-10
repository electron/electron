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
class URLRequestContextGetter;
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

  enum {
    OK = 0,
    ERR_SCHEME_REGISTERED,
    ERR_SCHEME_UNREGISTERED,
    ERR_SCHEME_INTERCEPTED,
    ERR_SCHEME_UNINTERCEPTED,
    ERR_NO_SCHEME,
    ERR_SCHEME
  };

  static mate::Handle<Protocol> Create(
      v8::Isolate* isolate, AtomBrowserContext* browser_context);

  JsProtocolHandler GetProtocolHandler(const std::string& scheme);

  net::URLRequestContextGetter* request_context_getter() {
    return request_context_getter_.get();
  }

 protected:
  explicit Protocol(AtomBrowserContext* browser_context);

  // mate::Wrappable implementations:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate);

 private:
  typedef std::map<std::string, JsProtocolHandler> ProtocolHandlersMap;

  // Callback called after performing action on IO thread.
  void OnIOActionCompleted(const JsCompletionCallback& callback,
                           int error);

  // Register schemes to standard scheme list.
  void RegisterStandardSchemes(const std::vector<std::string>& schemes);

  // Returns whether a scheme has been registered.
  void IsHandledProtocol(const std::string& scheme,
                         const net::CompletionCallback& callback);

  // Register/unregister an networking |scheme| which would be handled by
  // |callback|.
  void RegisterProtocol(v8::Isolate* isolate,
                        const std::string& scheme,
                        const JsProtocolHandler& handler,
                        const JsCompletionCallback& callback);
  void UnregisterProtocol(v8::Isolate* isolate, const std::string& scheme,
                          const JsCompletionCallback& callback);

  // Intercept/unintercept an existing protocol handler.
  void InterceptProtocol(v8::Isolate* isolate,
                         const std::string& scheme,
                         const JsProtocolHandler& handler,
                         const JsCompletionCallback& callback);
  void UninterceptProtocol(v8::Isolate* isolate, const std::string& scheme,
                           const JsCompletionCallback& callback);

  // The networking related operations have to be done in IO thread.
  int RegisterProtocolInIO(const std::string& scheme,
                           const JsProtocolHandler& handler);
  int UnregisterProtocolInIO(const std::string& scheme);
  int InterceptProtocolInIO(const std::string& scheme,
                            const JsProtocolHandler& handler);
  int UninterceptProtocolInIO(const std::string& scheme);

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  AtomURLRequestJobFactory* job_factory_;
  ProtocolHandlersMap protocol_handlers_;

  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
