// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_protocol.h"

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/stl_util.h"
#include "content/public/browser/child_process_security_policy.h"
#include "gin/object_template_builder.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/protocol_registry.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "url/url_util.h"

namespace {

// List of registered custom standard schemes.
std::vector<std::string> g_standard_schemes;

// List of registered custom streaming schemes.
std::vector<std::string> g_streaming_schemes;

struct SchemeOptions {
  bool standard = false;
  bool secure = false;
  bool bypassCSP = false;
  bool allowServiceWorkers = false;
  bool supportFetchAPI = false;
  bool corsEnabled = false;
  bool stream = false;
};

struct CustomScheme {
  std::string scheme;
  SchemeOptions options;
};

}  // namespace

namespace gin {

template <>
struct Converter<CustomScheme> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     CustomScheme* out) {
    gin::Dictionary dict(isolate);
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    if (!dict.Get("scheme", &(out->scheme)))
      return false;
    gin::Dictionary opt(isolate);
    // options are optional. Default values specified in SchemeOptions are used
    if (dict.Get("privileges", &opt)) {
      opt.Get("standard", &(out->options.standard));
      opt.Get("supportFetchAPI", &(out->options.supportFetchAPI));
      opt.Get("secure", &(out->options.secure));
      opt.Get("bypassCSP", &(out->options.bypassCSP));
      opt.Get("allowServiceWorkers", &(out->options.allowServiceWorkers));
      opt.Get("supportFetchAPI", &(out->options.supportFetchAPI));
      opt.Get("corsEnabled", &(out->options.corsEnabled));
      opt.Get("stream", &(out->options.stream));
    }
    return true;
  }
};

}  // namespace gin

namespace electron {
namespace api {

gin::WrapperInfo Protocol::kWrapperInfo = {gin::kEmbedderNativeGin};

std::vector<std::string> GetStandardSchemes() {
  return g_standard_schemes;
}

void RegisterSchemesAsPrivileged(gin_helper::ErrorThrower thrower,
                                 v8::Local<v8::Value> val) {
  std::vector<CustomScheme> custom_schemes;
  if (!gin::ConvertFromV8(thrower.isolate(), val, &custom_schemes)) {
    thrower.ThrowError("Argument must be an array of custom schemes.");
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
    if (custom_scheme.options.stream) {
      g_streaming_schemes.push_back(custom_scheme.scheme);
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
  AppendSchemesToCmdLine(electron::switches::kStreamingSchemes,
                         g_streaming_schemes);
}

namespace {

const char* kBuiltinSchemes[] = {
    "about", "file", "http", "https", "data", "filesystem",
};

// Convert error code to string.
std::string ErrorCodeToString(ProtocolError error) {
  switch (error) {
    case ProtocolError::kRegistered:
      return "The scheme has been registered";
    case ProtocolError::kNotRegistered:
      return "The scheme has not been registered";
    case ProtocolError::kIntercepted:
      return "The scheme has been intercepted";
    case ProtocolError::kNotIntercepted:
      return "The scheme has not been intercepted";
    default:
      return "Unexpected error";
  }
}

}  // namespace

Protocol::Protocol(v8::Isolate* isolate, ProtocolRegistry* protocol_registry)
    : protocol_registry_(protocol_registry) {}

Protocol::~Protocol() = default;

ProtocolError Protocol::RegisterProtocol(ProtocolType type,
                                         const std::string& scheme,
                                         const ProtocolHandler& handler) {
  bool added = protocol_registry_->RegisterProtocol(type, scheme, handler);
  return added ? ProtocolError::kOK : ProtocolError::kRegistered;
}

bool Protocol::UnregisterProtocol(const std::string& scheme,
                                  gin::Arguments* args) {
  bool removed = protocol_registry_->UnregisterProtocol(scheme);
  HandleOptionalCallback(
      args, removed ? ProtocolError::kOK : ProtocolError::kNotRegistered);
  return removed;
}

bool Protocol::IsProtocolRegistered(const std::string& scheme) {
  return protocol_registry_->IsProtocolRegistered(scheme);
}

ProtocolError Protocol::InterceptProtocol(ProtocolType type,
                                          const std::string& scheme,
                                          const ProtocolHandler& handler,
                                          std::set<URLPattern> url_patterns) {
  bool added = protocol_registry_->InterceptProtocol(type, scheme, handler,
                                                     url_patterns);
  return added ? ProtocolError::kOK : ProtocolError::kIntercepted;
}

bool Protocol::UninterceptProtocol(const std::string& scheme,
                                   gin::Arguments* args) {
  bool removed = protocol_registry_->UninterceptProtocol(scheme);
  HandleOptionalCallback(
      args, removed ? ProtocolError::kOK : ProtocolError::kNotIntercepted);
  return removed;
}

bool Protocol::IsProtocolIntercepted(const std::string& scheme) {
  return protocol_registry_->IsProtocolIntercepted(scheme);
}

v8::Local<v8::Promise> Protocol::IsProtocolHandled(const std::string& scheme,
                                                   gin::Arguments* args) {
  node::Environment* env = node::Environment::GetCurrent(args->isolate());
  EmitWarning(env,
              "The protocol.isProtocolHandled API is deprecated, use "
              "protocol.isProtocolRegistered or protocol.isProtocolIntercepted "
              "instead.",
              "ProtocolDeprecateIsProtocolHandled");
  return gin_helper::Promise<bool>::ResolvedPromise(
      args->isolate(),
      IsProtocolRegistered(scheme) || IsProtocolIntercepted(scheme) ||
          // The |isProtocolHandled| should return true for builtin
          // schemes, however with NetworkService it is impossible to
          // know which schemes are registered until a real network
          // request is sent.
          // So we have to test against a hard-coded builtin schemes
          // list make it work with old code. We should deprecate
          // this API with the new |isProtocolRegistered| API.
          base::Contains(kBuiltinSchemes, scheme));
}

void Protocol::HandleOptionalCallback(gin::Arguments* args,
                                      ProtocolError error) {
  base::RepeatingCallback<void(v8::Local<v8::Value>)> callback;
  if (args->GetNext(&callback)) {
    node::Environment* env = node::Environment::GetCurrent(args->isolate());
    EmitWarning(
        env,
        "The callback argument of protocol module APIs is no longer needed.",
        "ProtocolDeprecateCallback");
    if (error == ProtocolError::kOK)
      callback.Run(v8::Null(args->isolate()));
    else
      callback.Run(v8::Exception::Error(
          gin::StringToV8(args->isolate(), ErrorCodeToString(error))));
  }
}

// static
gin::Handle<Protocol> Protocol::Create(
    v8::Isolate* isolate,
    ElectronBrowserContext* browser_context) {
  return gin::CreateHandle(
      isolate, new Protocol(isolate, browser_context->protocol_registry()));
}

gin::ObjectTemplateBuilder Protocol::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<Protocol>::GetObjectTemplateBuilder(isolate)
      .SetMethod("registerStringProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kString>)
      .SetMethod("registerBufferProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kBuffer>)
      .SetMethod("registerFileProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kFile>)
      .SetMethod("registerHttpProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kHttp>)
      .SetMethod("registerStreamProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kStream>)
      .SetMethod("registerProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kFree>)
      .SetMethod("unregisterProtocol", &Protocol::UnregisterProtocol)
      .SetMethod("isProtocolRegistered", &Protocol::IsProtocolRegistered)
      .SetMethod("isProtocolHandled", &Protocol::IsProtocolHandled)
      .SetMethod("interceptStringProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kString>)
      .SetMethod("interceptBufferProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kBuffer>)
      .SetMethod("interceptFileProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kFile>)
      .SetMethod("interceptHttpProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kHttp>)
      .SetMethod("interceptStreamProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kStream>)
      .SetMethod("interceptProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kFree>)
      .SetMethod("uninterceptProtocol", &Protocol::UninterceptProtocol)
      .SetMethod("isProtocolIntercepted", &Protocol::IsProtocolIntercepted);
}

const char* Protocol::GetTypeName() {
  return "Protocol";
}

}  // namespace api
}  // namespace electron

namespace {

void RegisterSchemesAsPrivileged(gin_helper::ErrorThrower thrower,
                                 v8::Local<v8::Value> val) {
  if (electron::Browser::Get()->is_ready()) {
    thrower.ThrowError(
        "protocol.registerSchemesAsPrivileged should be called before "
        "app is ready");
    return;
  }

  electron::api::RegisterSchemesAsPrivileged(thrower, val);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("registerSchemesAsPrivileged", &RegisterSchemesAsPrivileged);
  dict.SetMethod("getStandardSchemes", &electron::api::GetStandardSchemes);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_protocol, Initialize)
