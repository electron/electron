// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_protocol.h"

#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/net/url_request_async_asar_job.h"
#include "atom/browser/net/url_request_buffer_job.h"
#include "atom/browser/net/url_request_fetch_job.h"
#include "atom/browser/net/url_request_string_job.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/net_converter.h"
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
    : request_context_getter_(browser_context->GetRequestContext()),
      job_factory_(browser_context->job_factory()) {
  CHECK(job_factory_);
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
                 base::Unretained(this), scheme),
      base::Bind(&Protocol::OnIOCompleted,
                 base::Unretained(this), callback));
}

Protocol::ProtocolError Protocol::UnregisterProtocolInIO(
    const std::string& scheme) {
  if (!job_factory_->HasProtocolHandler(scheme))
    return PROTOCOL_NOT_REGISTERED;
  job_factory_->SetProtocolHandler(scheme, nullptr);
  return PROTOCOL_OK;
}

void Protocol::IsProtocolHandled(const std::string& scheme,
                                    const BooleanCallback& callback) {
  content::BrowserThread::PostTaskAndReplyWithResult(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::IsProtocolHandledInIO,
                 base::Unretained(this), scheme),
      callback);
}

bool Protocol::IsProtocolHandledInIO(const std::string& scheme) {
  return job_factory_->IsHandledProtocol(scheme);
}

void Protocol::UninterceptProtocol(
    const std::string& scheme, mate::Arguments* args) {
  CompletionCallback callback;
  args->GetNext(&callback);
  content::BrowserThread::PostTaskAndReplyWithResult(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::UninterceptProtocolInIO,
                 base::Unretained(this), scheme),
      base::Bind(&Protocol::OnIOCompleted,
                 base::Unretained(this), callback));
}

Protocol::ProtocolError Protocol::UninterceptProtocolInIO(
    const std::string& scheme) {
  if (!original_protocols_.contains(scheme))
    return PROTOCOL_NOT_INTERCEPTED;
  job_factory_->ReplaceProtocol(scheme,
                                original_protocols_.take_and_erase(scheme));
  return PROTOCOL_OK;
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

mate::Handle<atom::api::Protocol> CreateProtocol(v8::Isolate* isolate) {
  auto browser_context = static_cast<atom::AtomBrowserContext*>(
      atom::AtomBrowserMainParts::Get()->browser_context());
  return atom::api::Protocol::Create(isolate, browser_context);
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("createProtocolObject", base::Bind(&CreateProtocol, isolate));
  dict.SetMethod("registerStandardSchemes", &RegisterStandardSchemes);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_protocol, Initialize)
