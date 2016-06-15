// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_protocol.h"

#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/net/url_request_async_asar_job.h"
#include "atom/browser/net/url_request_buffer_job.h"
#include "atom/browser/net/url_request_fetch_job.h"
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

namespace atom {

namespace api {

Protocol::Protocol(v8::Isolate* isolate, AtomBrowserContext* browser_context)
    : request_context_getter_(static_cast<brightray::URLRequestContextGetter*>(
          browser_context->GetRequestContext())),
      weak_factory_(this) {
  Init(isolate);
}

void Protocol::RegisterServiceWorkerSchemes(
    const std::vector<std::string>& schemes) {
  atom::AtomBrowserClient::SetCustomServiceWorkerSchemes(schemes);
}

void Protocol::UnregisterProtocol(
    const std::string& scheme, mate::Arguments* args) {
  CompletionCallback callback;
  args->GetNext(&callback);
  content::BrowserThread::PostTaskAndReplyWithResult(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::UnregisterProtocolInIO,
                 request_context_getter_, scheme),
      base::Bind(&Protocol::OnIOCompleted,
                 GetWeakPtr(), callback));
}

// static
Protocol::ProtocolError Protocol::UnregisterProtocolInIO(
    scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
    const std::string& scheme) {
  auto job_factory = static_cast<AtomURLRequestJobFactory*>(
      request_context_getter->job_factory());
  if (!job_factory->HasProtocolHandler(scheme))
    return PROTOCOL_NOT_REGISTERED;
  job_factory->SetProtocolHandler(scheme, nullptr);
  return PROTOCOL_OK;
}

void Protocol::IsProtocolHandled(const std::string& scheme,
                                 const BooleanCallback& callback) {
  content::BrowserThread::PostTaskAndReplyWithResult(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::IsProtocolHandledInIO,
                 request_context_getter_, scheme),
      callback);
}

// static
bool Protocol::IsProtocolHandledInIO(
    scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
    const std::string& scheme) {
  return request_context_getter->job_factory()->IsHandledProtocol(scheme);
}

void Protocol::UninterceptProtocol(
    const std::string& scheme, mate::Arguments* args) {
  CompletionCallback callback;
  args->GetNext(&callback);
  content::BrowserThread::PostTaskAndReplyWithResult(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::UninterceptProtocolInIO,
                 request_context_getter_, scheme),
      base::Bind(&Protocol::OnIOCompleted,
                 GetWeakPtr(), callback));
}

// static
Protocol::ProtocolError Protocol::UninterceptProtocolInIO(
    scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
    const std::string& scheme) {
  return static_cast<AtomURLRequestJobFactory*>(
      request_context_getter->job_factory())->UninterceptProtocol(scheme) ?
          PROTOCOL_OK : PROTOCOL_NOT_INTERCEPTED;
}

void Protocol::OnIOCompleted(
    const CompletionCallback& callback, ProtocolError error) {
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
    case PROTOCOL_FAIL: return "Failed to manipulate protocol factory";
    case PROTOCOL_REGISTERED: return "The scheme has been registred";
    case PROTOCOL_NOT_REGISTERED: return "The scheme has not been registred";
    case PROTOCOL_INTERCEPTED: return "The scheme has been intercepted";
    case PROTOCOL_NOT_INTERCEPTED: return "The scheme has not been intercepted";
    default: return "Unexpected error";
  }
}

AtomURLRequestJobFactory* Protocol::GetJobFactoryInIO() const {
  request_context_getter_->GetURLRequestContext();  // Force init.
  return static_cast<AtomURLRequestJobFactory*>(
      static_cast<brightray::URLRequestContextGetter*>(
          request_context_getter_.get())->job_factory());
}

// static
mate::Handle<Protocol> Protocol::Create(
    v8::Isolate* isolate, AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new Protocol(isolate, browser_context));
}

// static
void Protocol::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("registerServiceWorkerSchemes",
                 &Protocol::RegisterServiceWorkerSchemes)
      .SetMethod("registerStringProtocol",
                 &Protocol::RegisterProtocol<URLRequestStringJob>)
      .SetMethod("registerBufferProtocol",
                 &Protocol::RegisterProtocol<URLRequestBufferJob>)
      .SetMethod("registerFileProtocol",
                 &Protocol::RegisterProtocol<URLRequestAsyncAsarJob>)
      .SetMethod("registerHttpProtocol",
                 &Protocol::RegisterProtocol<URLRequestFetchJob>)
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
      .SetMethod("uninterceptProtocol", &Protocol::UninterceptProtocol);
}

}  // namespace api

}  // namespace atom

namespace {

void RegisterStandardSchemes(
    const std::vector<std::string>& schemes) {
  auto policy = content::ChildProcessSecurityPolicy::GetInstance();
  for (const auto& scheme : schemes) {
    url::AddStandardScheme(scheme.c_str(), url::SCHEME_WITHOUT_PORT);
    policy->RegisterWebSafeScheme(scheme);
  }

  auto command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII(atom::switches::kStandardSchemes,
                                  base::JoinString(schemes, ","));
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("registerStandardSchemes", &RegisterStandardSchemes);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_protocol, Initialize)
