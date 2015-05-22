// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
#define ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_

#include <string>
#include <map>

#include "atom/browser/api/event_emitter.h"
#include "base/callback.h"
#include "native_mate/handle.h"

namespace net {
class URLRequest;
}

namespace atom {

class AtomURLRequestJobFactory;

namespace api {

class Protocol : public mate::EventEmitter {
 public:
  typedef base::Callback<v8::Local<v8::Value>(const net::URLRequest*)>
          JsProtocolHandler;

  static mate::Handle<Protocol> Create(v8::Isolate* isolate);

  JsProtocolHandler GetProtocolHandler(const std::string& scheme);

 protected:
  Protocol();

  // mate::Wrappable implementations:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate);

 private:
  typedef std::map<std::string, JsProtocolHandler> ProtocolHandlersMap;

  // Register/unregister an networking |scheme| which would be handled by
  // |callback|.
  void RegisterProtocol(const std::string& scheme,
                        const JsProtocolHandler& callback);
  void UnregisterProtocol(const std::string& scheme);

  // Returns whether a scheme has been registered.
  // FIXME Should accept a callback and be asynchronous so we do not have to use
  // locks.
  bool IsHandledProtocol(const std::string& scheme);

  // Intercept/unintercept an existing protocol handler.
  void InterceptProtocol(const std::string& scheme,
                         const JsProtocolHandler& callback);
  void UninterceptProtocol(const std::string& scheme);

  // The networking related operations have to be done in IO thread.
  void RegisterProtocolInIO(const std::string& scheme);
  void UnregisterProtocolInIO(const std::string& scheme);
  void InterceptProtocolInIO(const std::string& scheme);
  void UninterceptProtocolInIO(const std::string& scheme);

  // Do protocol.emit(event, parameter) under UI thread.
  void EmitEventInUI(const std::string& event, const std::string& parameter);

  AtomURLRequestJobFactory* job_factory_;
  ProtocolHandlersMap protocol_handlers_;

  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
