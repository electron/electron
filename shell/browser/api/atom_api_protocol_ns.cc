// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_protocol_ns.h"

#include <memory>
#include <utility>

#include "base/stl_util.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/deprecate_util.h"
#include "shell/common/native_mate_converters/net_converter.h"
#include "shell/common/native_mate_converters/once_callback.h"
#include "shell/common/promise_util.h"

namespace electron {
namespace api {

namespace {

const char* kBuiltinSchemes[] = {
    "about", "file", "http", "https", "data", "filesystem",
};

// Convert error code to string.
std::string ErrorCodeToString(ProtocolError error) {
  switch (error) {
    case ProtocolError::REGISTERED:
      return "The scheme has been registered";
    case ProtocolError::NOT_REGISTERED:
      return "The scheme has not been registered";
    case ProtocolError::INTERCEPTED:
      return "The scheme has been intercepted";
    case ProtocolError::NOT_INTERCEPTED:
      return "The scheme has not been intercepted";
    default:
      return "Unexpected error";
  }
}

}  // namespace

ProtocolNS::ProtocolNS(v8::Isolate* isolate,
                       AtomBrowserContext* browser_context) {
  Init(isolate);
  AttachAsUserData(browser_context);
}

ProtocolNS::~ProtocolNS() = default;

void ProtocolNS::RegisterURLLoaderFactories(
    content::ContentBrowserClient::NonNetworkURLLoaderFactoryMap* factories) {
  for (const auto& it : handlers_) {
    factories->emplace(it.first, std::make_unique<AtomURLLoaderFactory>(
                                     it.second.first, it.second.second));
  }
}

ProtocolError ProtocolNS::RegisterProtocol(ProtocolType type,
                                           const std::string& scheme,
                                           const ProtocolHandler& handler) {
  ProtocolError error = ProtocolError::OK;
  if (!base::Contains(handlers_, scheme))
    handlers_[scheme] = std::make_pair(type, handler);
  else
    error = ProtocolError::REGISTERED;
  return error;
}

void ProtocolNS::UnregisterProtocol(const std::string& scheme,
                                    mate::Arguments* args) {
  ProtocolError error = ProtocolError::OK;
  if (base::Contains(handlers_, scheme))
    handlers_.erase(scheme);
  else
    error = ProtocolError::NOT_REGISTERED;
  HandleOptionalCallback(args, error);
}

bool ProtocolNS::IsProtocolRegistered(const std::string& scheme) {
  return base::Contains(handlers_, scheme);
}

ProtocolError ProtocolNS::InterceptProtocol(ProtocolType type,
                                            const std::string& scheme,
                                            const ProtocolHandler& handler) {
  ProtocolError error = ProtocolError::OK;
  if (!base::Contains(intercept_handlers_, scheme))
    intercept_handlers_[scheme] = std::make_pair(type, handler);
  else
    error = ProtocolError::INTERCEPTED;
  return error;
}

void ProtocolNS::UninterceptProtocol(const std::string& scheme,
                                     mate::Arguments* args) {
  ProtocolError error = ProtocolError::OK;
  if (base::Contains(intercept_handlers_, scheme))
    intercept_handlers_.erase(scheme);
  else
    error = ProtocolError::NOT_INTERCEPTED;
  HandleOptionalCallback(args, error);
}

bool ProtocolNS::IsProtocolIntercepted(const std::string& scheme) {
  return base::Contains(intercept_handlers_, scheme);
}

v8::Local<v8::Promise> ProtocolNS::IsProtocolHandled(const std::string& scheme,
                                                     mate::Arguments* args) {
  node::Environment* env = node::Environment::GetCurrent(args->isolate());
  EmitDeprecationWarning(
      env,
      "The protocol.isProtocolHandled API is deprecated, use "
      "protocol.isProtocolRegistered or protocol.isProtocolIntercepted "
      "instead.",
      "ProtocolDeprecateIsProtocolHandled");
  util::Promise promise(isolate());
  promise.Resolve(IsProtocolRegistered(scheme) ||
                  IsProtocolIntercepted(scheme) ||
                  // The |isProtocolHandled| should return true for builtin
                  // schemes, however with NetworkService it is impossible to
                  // know which schemes are registered until a real network
                  // request is sent.
                  // So we have to test against a hard-coded builtin schemes
                  // list make it work with old code. We should deprecate this
                  // API with the new |isProtocolRegistered| API.
                  base::Contains(kBuiltinSchemes, scheme));
  return promise.GetHandle();
}

void ProtocolNS::HandleOptionalCallback(mate::Arguments* args,
                                        ProtocolError error) {
  CompletionCallback callback;
  if (args->GetNext(&callback)) {
    node::Environment* env = node::Environment::GetCurrent(args->isolate());
    EmitDeprecationWarning(
        env,
        "The callback argument of protocol module APIs is no longer needed.",
        "ProtocolDeprecateCallback");
    if (error == ProtocolError::OK)
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
      .SetMethod("registerStringProtocol",
                 &ProtocolNS::RegisterProtocolFor<ProtocolType::kString>)
      .SetMethod("registerBufferProtocol",
                 &ProtocolNS::RegisterProtocolFor<ProtocolType::kBuffer>)
      .SetMethod("registerFileProtocol",
                 &ProtocolNS::RegisterProtocolFor<ProtocolType::kFile>)
      .SetMethod("registerHttpProtocol",
                 &ProtocolNS::RegisterProtocolFor<ProtocolType::kHttp>)
      .SetMethod("registerStreamProtocol",
                 &ProtocolNS::RegisterProtocolFor<ProtocolType::kStream>)
      .SetMethod("registerProtocol",
                 &ProtocolNS::RegisterProtocolFor<ProtocolType::kFree>)
      .SetMethod("unregisterProtocol", &ProtocolNS::UnregisterProtocol)
      .SetMethod("isProtocolRegistered", &ProtocolNS::IsProtocolRegistered)
      .SetMethod("isProtocolHandled", &ProtocolNS::IsProtocolHandled)
      .SetMethod("interceptStringProtocol",
                 &ProtocolNS::InterceptProtocolFor<ProtocolType::kString>)
      .SetMethod("interceptBufferProtocol",
                 &ProtocolNS::InterceptProtocolFor<ProtocolType::kBuffer>)
      .SetMethod("interceptFileProtocol",
                 &ProtocolNS::InterceptProtocolFor<ProtocolType::kFile>)
      .SetMethod("interceptHttpProtocol",
                 &ProtocolNS::InterceptProtocolFor<ProtocolType::kHttp>)
      .SetMethod("interceptStreamProtocol",
                 &ProtocolNS::InterceptProtocolFor<ProtocolType::kStream>)
      .SetMethod("interceptProtocol",
                 &ProtocolNS::InterceptProtocolFor<ProtocolType::kFree>)
      .SetMethod("uninterceptProtocol", &ProtocolNS::UninterceptProtocol)
      .SetMethod("isProtocolIntercepted", &ProtocolNS::IsProtocolIntercepted);
}

}  // namespace api
}  // namespace electron
