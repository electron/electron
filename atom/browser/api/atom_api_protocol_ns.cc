// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_protocol_ns.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_url_loader_factory.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/stl_util.h"

namespace atom {
namespace api {

namespace {

// Convert error code to string.
std::string ErrorCodeToString(ProtocolError error) {
  switch (error) {
    case PROTOCOL_REGISTERED:
      return "The scheme has been registered";
    case PROTOCOL_NOT_REGISTERED:
      return "The scheme has not been registered";
    case PROTOCOL_INTERCEPTED:
      return "The scheme has been intercepted";
    case PROTOCOL_NOT_INTERCEPTED:
      return "The scheme has not been intercepted";
    default:
      return "Unexpected error";
  }
}

void Noop() {}

}  // namespace

ProtocolNS::ProtocolNS(v8::Isolate* isolate,
                       AtomBrowserContext* browser_context) {
  Init(isolate);
  AttachAsUserData(browser_context);
}

ProtocolNS::~ProtocolNS() = default;

void ProtocolNS::RegisterURLLoaderFactories(
    content::ContentBrowserClient::NonNetworkURLLoaderFactoryMap* factories) {
  for (const auto& it : handlers_)
    factories->emplace(it.first, std::make_unique<AtomURLLoaderFactory>());
}

int ProtocolNS::RegisterProtocol(const std::string& scheme,
                                 const Handler& handler,
                                 mate::Arguments* args) {
  ProtocolError error = PROTOCOL_OK;
  if (base::ContainsKey(handlers_, scheme))
    error = PROTOCOL_REGISTERED;
  else
    handlers_[scheme] = handler;

  // Be compatible with old interface, which accepts optional callback.
  CompletionCallback callback;
  if (args->GetNext(&callback)) {
    if (error == PROTOCOL_OK)
      callback.Run(v8::Null(args->isolate()));
    else
      callback.Run(v8::Exception::Error(
          mate::StringToV8(isolate(), ErrorCodeToString(error))));
  }
  return error;
}

// static
mate::Handle<ProtocolNS> ProtocolNS::Create(
    v8::Isolate* isolate,
    AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new ProtocolNS(isolate, browser_context));
}

// static
void ProtocolNS::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Protocol"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("registerStringProtocol", &ProtocolNS::RegisterProtocol)
      .SetMethod("registerBufferProtocol", &ProtocolNS::RegisterProtocol)
      .SetMethod("registerFileProtocol", &ProtocolNS::RegisterProtocol)
      .SetMethod("registerHttpProtocol", &ProtocolNS::RegisterProtocol)
      .SetMethod("registerStreamProtocol", &ProtocolNS::RegisterProtocol)
      .SetMethod("unregisterProtocol", &Noop)
      .SetMethod("isProtocolHandled", &Noop)
      .SetMethod("interceptStringProtocol", &Noop)
      .SetMethod("interceptBufferProtocol", &Noop)
      .SetMethod("interceptFileProtocol", &Noop)
      .SetMethod("interceptHttpProtocol", &Noop)
      .SetMethod("interceptStreamProtocol", &Noop)
      .SetMethod("uninterceptProtocol", &Noop);
}

}  // namespace api
}  // namespace atom
