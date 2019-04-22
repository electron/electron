// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_protocol_ns.h"

#include <memory>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_url_loader_factory.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/promise_util.h"
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
  if (!base::ContainsKey(handlers_, scheme))
    handlers_[scheme] = handler;
  else
    error = PROTOCOL_REGISTERED;
  HandleOptionalCallback(args, error);
  return error;
}

void ProtocolNS::UnregisterProtocol(const std::string& scheme,
                                    mate::Arguments* args) {
  ProtocolError error = PROTOCOL_OK;
  if (base::ContainsKey(handlers_, scheme))
    handlers_.erase(scheme);
  else
    error = PROTOCOL_NOT_REGISTERED;
  HandleOptionalCallback(args, error);
}

bool ProtocolNS::IsProtocolRegistered(const std::string& scheme) {
  return base::ContainsKey(handlers_, scheme);
}

v8::Local<v8::Promise> ProtocolNS::IsProtocolHandled(
    const std::string& scheme) {
  util::Promise promise(isolate());
  promise.Resolve(IsProtocolRegistered(scheme));
  return promise.GetHandle();
}

void ProtocolNS::HandleOptionalCallback(mate::Arguments* args,
                                        ProtocolError error) {
  CompletionCallback callback;
  if (args->GetNext(&callback)) {
    if (error == PROTOCOL_OK)
      callback.Run(v8::Null(args->isolate()));
    else
      callback.Run(v8::Exception::Error(
          mate::StringToV8(isolate(), ErrorCodeToString(error))));
  }
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
      .SetMethod("unregisterProtocol", &ProtocolNS::UnregisterProtocol)
      .SetMethod("isProtocolRegistered", &ProtocolNS::IsProtocolRegistered)
      .SetMethod("isProtocolHandled", &ProtocolNS::IsProtocolHandled)
      .SetMethod("interceptStringProtocol", &Noop)
      .SetMethod("interceptBufferProtocol", &Noop)
      .SetMethod("interceptFileProtocol", &Noop)
      .SetMethod("interceptHttpProtocol", &Noop)
      .SetMethod("interceptStreamProtocol", &Noop)
      .SetMethod("uninterceptProtocol", &Noop);
}

}  // namespace api
}  // namespace atom
