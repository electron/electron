// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_PROTOCOL_NS_H_
#define ATOM_BROWSER_API_ATOM_API_PROTOCOL_NS_H_

#include <map>
#include <string>
#include <utility>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/net/atom_url_loader_factory.h"
#include "content/public/browser/content_browser_client.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"

namespace atom {

class AtomBrowserContext;

namespace api {

// Possible errors.
enum ProtocolError {
  PROTOCOL_OK,  // no error
  PROTOCOL_REGISTERED,
  PROTOCOL_NOT_REGISTERED,
  PROTOCOL_INTERCEPTED,
  PROTOCOL_NOT_INTERCEPTED,
};

// Protocol implementation based on network services.
class ProtocolNS : public mate::TrackableObject<ProtocolNS> {
 public:
  static mate::Handle<ProtocolNS> Create(v8::Isolate* isolate,
                                         AtomBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Used by AtomBrowserClient for creating URLLoaderFactory.
  void RegisterURLLoaderFactories(
      content::ContentBrowserClient::NonNetworkURLLoaderFactoryMap* factories);

 private:
  ProtocolNS(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~ProtocolNS() override;

  // Callback types.
  using CompletionCallback = base::Callback<void(v8::Local<v8::Value>)>;

  // JS APIs.
  ProtocolError RegisterProtocol(ProtocolType type,
                                 const std::string& scheme,
                                 const ProtocolHandler& handler);
  void UnregisterProtocol(const std::string& scheme, mate::Arguments* args);
  bool IsProtocolRegistered(const std::string& scheme);

  // Old async version of IsProtocolRegistered.
  v8::Local<v8::Promise> IsProtocolHandled(const std::string& scheme);

  // Helper for converting old registration APIs to new RegisterProtocol API.
  template <ProtocolType type>
  void RegisterProtocolFor(const std::string& scheme,
                           const ProtocolHandler& handler,
                           mate::Arguments* args) {
    HandleOptionalCallback(args, RegisterProtocol(type, scheme, handler));
  }

  // Be compatible with old interface, which accepts optional callback.
  void HandleOptionalCallback(mate::Arguments* args, ProtocolError error);

  // scheme => (type, handler).
  std::map<std::string, std::pair<ProtocolType, ProtocolHandler>> handlers_;
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_PROTOCOL_NS_H_
