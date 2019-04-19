// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_protocol.h"

#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/browser.h"
#include "atom/browser/net/url_request_async_asar_job.h"
#include "atom/browser/net/url_request_buffer_job.h"
#include "atom/browser/net/url_request_fetch_job.h"
#include "atom/browser/net/url_request_stream_job.h"
#include "atom/browser/net/url_request_string_job.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "content/public/browser/child_process_security_policy.h"
#include "native_mate/dictionary.h"
#include "url/url_util.h"

using content::BrowserThread;

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

namespace atom {

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

  AppendSchemesToCmdLine(atom::switches::kSecureSchemes, secure_schemes);
  AppendSchemesToCmdLine(atom::switches::kBypassCSPSchemes,
                         cspbypassing_schemes);
  AppendSchemesToCmdLine(atom::switches::kCORSSchemes, cors_schemes);
  AppendSchemesToCmdLine(atom::switches::kFetchSchemes, fetch_schemes);
  AppendSchemesToCmdLine(atom::switches::kServiceWorkerSchemes,
                         service_worker_schemes);
  AppendSchemesToCmdLine(atom::switches::kStandardSchemes, g_standard_schemes);
}

Protocol::Protocol(v8::Isolate* isolate, AtomBrowserContext* browser_context)
    : browser_context_(browser_context), weak_factory_(this) {
  Init(isolate);
}

Protocol::~Protocol() {}
void Protocol::UnregisterProtocol(const std::string& scheme,
                                  mate::Arguments* args) {
  CompletionCallback callback;
  args->GetNext(&callback);
  auto* getter = static_cast<URLRequestContextGetter*>(
      browser_context_->GetRequestContext());
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&Protocol::UnregisterProtocolInIO,
                     base::RetainedRef(getter), scheme),
      base::BindOnce(&Protocol::OnIOCompleted, GetWeakPtr(), callback));
}

// static
Protocol::ProtocolError Protocol::UnregisterProtocolInIO(
    scoped_refptr<URLRequestContextGetter> request_context_getter,
    const std::string& scheme) {
  auto* job_factory = request_context_getter->job_factory();
  if (!job_factory->HasProtocolHandler(scheme))
    return PROTOCOL_NOT_REGISTERED;
  job_factory->SetProtocolHandler(scheme, nullptr);
  return PROTOCOL_OK;
}

bool IsProtocolHandledInIO(
    scoped_refptr<URLRequestContextGetter> request_context_getter,
    const std::string& scheme) {
  bool is_handled =
      request_context_getter->job_factory()->IsHandledProtocol(scheme);
  return is_handled;
}

void PromiseCallback(util::Promise promise, bool handled) {
  promise.Resolve(handled);
}

v8::Local<v8::Promise> Protocol::IsProtocolHandled(const std::string& scheme) {
  util::Promise promise(isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();
  auto* getter = static_cast<URLRequestContextGetter*>(
      browser_context_->GetRequestContext());

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&IsProtocolHandledInIO, base::RetainedRef(getter), scheme),
      base::BindOnce(&PromiseCallback, std::move(promise)));

  return handle;
}

void Protocol::UninterceptProtocol(const std::string& scheme,
                                   mate::Arguments* args) {
  CompletionCallback callback;
  args->GetNext(&callback);
  auto* getter = static_cast<URLRequestContextGetter*>(
      browser_context_->GetRequestContext());
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&Protocol::UninterceptProtocolInIO,
                     base::RetainedRef(getter), scheme),
      base::BindOnce(&Protocol::OnIOCompleted, GetWeakPtr(), callback));
}

// static
Protocol::ProtocolError Protocol::UninterceptProtocolInIO(
    scoped_refptr<URLRequestContextGetter> request_context_getter,
    const std::string& scheme) {
  return request_context_getter->job_factory()->UninterceptProtocol(scheme)
             ? PROTOCOL_OK
             : PROTOCOL_NOT_INTERCEPTED;
}

void Protocol::OnIOCompleted(const CompletionCallback& callback,
                             ProtocolError error) {
  // The completion callback is optional.
  if (callback.is_null())
    return;

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());

  if (error == PROTOCOL_OK) {
    callback.Run(v8::Null(isolate()));
  } else {
    std::string str = ErrorCodeToString(error);
    callback.Run(v8::Exception::Error(mate::StringToV8(isolate(), str)));
  }
}

std::string Protocol::ErrorCodeToString(ProtocolError error) {
  switch (error) {
    case PROTOCOL_FAIL:
      return "Failed to manipulate protocol factory";
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

// static
mate::Handle<Protocol> Protocol::Create(v8::Isolate* isolate,
                                        AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new Protocol(isolate, browser_context));
}

// static
void Protocol::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Protocol"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("registerStringProtocol",
                 &Protocol::RegisterProtocol<URLRequestStringJob>)
      .SetMethod("registerBufferProtocol",
                 &Protocol::RegisterProtocol<URLRequestBufferJob>)
      .SetMethod("registerFileProtocol",
                 &Protocol::RegisterProtocol<URLRequestAsyncAsarJob>)
      .SetMethod("registerHttpProtocol",
                 &Protocol::RegisterProtocol<URLRequestFetchJob>)
      .SetMethod("registerStreamProtocol",
                 &Protocol::RegisterProtocol<URLRequestStreamJob>)
      .SetMethod("unregisterProtocol", &Protocol::UnregisterProtocol)
      .SetMethod("isProtocolHandled", &Protocol::IsProtocolHandled)
      .SetMethod("interceptStringProtocol",
                 &Protocol::InterceptProtocol<URLRequestStringJob>)
      .SetMethod("interceptBufferProtocol",
                 &Protocol::InterceptProtocol<URLRequestBufferJob>)
      .SetMethod("interceptFileProtocol",
                 &Protocol::InterceptProtocol<URLRequestAsyncAsarJob>)
      .SetMethod("interceptHttpProtocol",
                 &Protocol::InterceptProtocol<URLRequestFetchJob>)
      .SetMethod("interceptStreamProtocol",
                 &Protocol::InterceptProtocol<URLRequestStreamJob>)
      .SetMethod("uninterceptProtocol", &Protocol::UninterceptProtocol);
}

}  // namespace api

}  // namespace atom

namespace {

void RegisterSchemesAsPrivileged(v8::Local<v8::Value> val,
                                 mate::Arguments* args) {
  if (atom::Browser::Get()->is_ready()) {
    args->ThrowError(
        "protocol.registerSchemesAsPrivileged should be called before "
        "app is ready");
    return;
  }

  atom::api::RegisterSchemesAsPrivileged(val, args);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("registerSchemesAsPrivileged", &RegisterSchemesAsPrivileged);
  dict.SetMethod("getStandardSchemes", &atom::api::GetStandardSchemes);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_protocol, Initialize)
