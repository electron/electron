// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_protocol_ns.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/stl_util.h"
#include "content/public/browser/child_process_security_policy.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/browser/browser.h"
#include "shell/common/deprecate_util.h"
#include "shell/common/native_mate_converters/net_converter.h"
#include "shell/common/native_mate_converters/once_callback.h"
#include "shell/common/options_switches.h"
#include "shell/common/promise_util.h"

namespace {

// List of registered custom standard schemes.
std::vector<std::string> g_standard_schemes;

struct SchemeOptions {
  bool standard = false;
  bool secure = false;
  bool bypassCSP = false;
  bool allowServiceWorkers = false;
  bool supportFetchAPI = false;
  bool corsEnabled = false;
};

struct CustomScheme {
  std::string scheme;
  SchemeOptions options;
};

}  // namespace

namespace mate {

template <>
struct Converter<CustomScheme> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     CustomScheme* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    if (!dict.Get("scheme", &(out->scheme)))
      return false;
    mate::Dictionary opt;
    // options are optional. Default values specified in SchemeOptions are used
    if (dict.Get("privileges", &opt)) {
      opt.Get("standard", &(out->options.standard));
      opt.Get("supportFetchAPI", &(out->options.supportFetchAPI));
      opt.Get("secure", &(out->options.secure));
      opt.Get("bypassCSP", &(out->options.bypassCSP));
      opt.Get("allowServiceWorkers", &(out->options.allowServiceWorkers));
      opt.Get("supportFetchAPI", &(out->options.supportFetchAPI));
      opt.Get("corsEnabled", &(out->options.corsEnabled));
    }
    return true;
  }
};

}  // namespace mate

namespace electron {
namespace api {

std::vector<std::string> GetStandardSchemes() {
  return g_standard_schemes;
}

void RegisterSchemesAsPrivileged(v8::Local<v8::Value> val,
                                 mate::Arguments* args) {
  std::vector<CustomScheme> custom_schemes;
  if (!mate::ConvertFromV8(args->isolate(), val, &custom_schemes)) {
    args->ThrowError("Argument must be an array of custom schemes.");
    return;
  }

  std::vector<std::string> secure_schemes, cspbypassing_schemes, fetch_schemes,
      service_worker_schemes, cors_schemes;
  for (const auto& custom_scheme : custom_schemes) {
    // Register scheme to privileged list (https, wss, data, chrome-extension)
    if (custom_scheme.options.standard) {
      auto* policy = content::ChildProcessSecurityPolicy::GetInstance();
      url::AddStandardScheme(custom_scheme.scheme.c_str(),
                             url::SCHEME_WITH_HOST);
      g_standard_schemes.push_back(custom_scheme.scheme);
      policy->RegisterWebSafeScheme(custom_scheme.scheme);
    }
    if (custom_scheme.options.secure) {
      secure_schemes.push_back(custom_scheme.scheme);
      url::AddSecureScheme(custom_scheme.scheme.c_str());
    }
    if (custom_scheme.options.bypassCSP) {
      cspbypassing_schemes.push_back(custom_scheme.scheme);
      url::AddCSPBypassingScheme(custom_scheme.scheme.c_str());
    }
    if (custom_scheme.options.corsEnabled) {
      cors_schemes.push_back(custom_scheme.scheme);
      url::AddCorsEnabledScheme(custom_scheme.scheme.c_str());
    }
    if (custom_scheme.options.supportFetchAPI) {
      fetch_schemes.push_back(custom_scheme.scheme);
    }
    if (custom_scheme.options.allowServiceWorkers) {
      service_worker_schemes.push_back(custom_scheme.scheme);
    }
  }

  const auto AppendSchemesToCmdLine = [](const char* switch_name,
                                         std::vector<std::string> schemes) {
    // Add the schemes to command line switches, so child processes can also
    // register them.
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switch_name, base::JoinString(schemes, ","));
  };

  AppendSchemesToCmdLine(electron::switches::kSecureSchemes, secure_schemes);
  AppendSchemesToCmdLine(electron::switches::kBypassCSPSchemes,
                         cspbypassing_schemes);
  AppendSchemesToCmdLine(electron::switches::kCORSSchemes, cors_schemes);
  AppendSchemesToCmdLine(electron::switches::kFetchSchemes, fetch_schemes);
  AppendSchemesToCmdLine(electron::switches::kServiceWorkerSchemes,
                         service_worker_schemes);
  AppendSchemesToCmdLine(electron::switches::kStandardSchemes,
                         g_standard_schemes);
}

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

namespace {

void RegisterSchemesAsPrivileged(v8::Local<v8::Value> val,
                                 mate::Arguments* args) {
  if (electron::Browser::Get()->is_ready()) {
    args->ThrowError(
        "protocol.registerSchemesAsPrivileged should be called before "
        "app is ready");
    return;
  }

  electron::api::RegisterSchemesAsPrivileged(val, args);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("registerSchemesAsPrivileged", &RegisterSchemesAsPrivileged);
  dict.SetMethod("getStandardSchemes", &electron::api::GetStandardSchemes);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_protocol, Initialize)
