// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_web_request_ns.h"

#include <memory>
#include <string>
#include <utility>

#include "gin/converter.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/gin_converters/callback_converter_gin_adapter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_converters/value_converter_gin_adapter.h"

namespace gin {

template <>
struct Converter<URLPattern> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     URLPattern* out) {
    std::string pattern;
    if (!ConvertFromV8(isolate, val, &pattern))
      return false;
    *out = URLPattern(URLPattern::SCHEME_ALL);
    return out->Parse(pattern) == URLPattern::ParseResult::kSuccess;
  }
};

}  // namespace gin

namespace electron {

namespace api {

namespace {

const char* kUserDataKey = "WebRequestNS";

// BrowserContext <=> WebRequestNS relationship.
struct UserData : public base::SupportsUserData::Data {
  explicit UserData(WebRequestNS* data) : data(data) {}
  WebRequestNS* data;
};

// Test whether the URL of |request| matches |patterns|.
bool MatchesFilterCondition(extensions::WebRequestInfo* info,
                            const std::set<URLPattern>& patterns) {
  if (patterns.empty())
    return true;

  for (const auto& pattern : patterns) {
    if (pattern.MatchesURL(info->url))
      return true;
  }
  return false;
}

}  // namespace

gin::WrapperInfo WebRequestNS::kWrapperInfo = {gin::kEmbedderNativeGin};

WebRequestNS::SimpleListenerInfo::SimpleListenerInfo(
    std::set<URLPattern> patterns_,
    SimpleListener listener_)
    : url_patterns(std::move(patterns_)), listener(listener_) {}
WebRequestNS::SimpleListenerInfo::SimpleListenerInfo() = default;
WebRequestNS::SimpleListenerInfo::~SimpleListenerInfo() = default;

WebRequestNS::ResponseListenerInfo::ResponseListenerInfo(
    std::set<URLPattern> patterns_,
    ResponseListener listener_)
    : url_patterns(std::move(patterns_)), listener(listener_) {}
WebRequestNS::ResponseListenerInfo::ResponseListenerInfo() = default;
WebRequestNS::ResponseListenerInfo::~ResponseListenerInfo() = default;

WebRequestNS::WebRequestNS(v8::Isolate* isolate,
                           content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  browser_context_->SetUserData(kUserDataKey, std::make_unique<UserData>(this));
}

WebRequestNS::~WebRequestNS() {
  browser_context_->RemoveUserData(kUserDataKey);
}

gin::ObjectTemplateBuilder WebRequestNS::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebRequestNS>::GetObjectTemplateBuilder(isolate)
      .SetMethod("onBeforeRequest",
                 &WebRequestNS::SetResponseListener<kOnBeforeRequest>)
      .SetMethod("onBeforeSendHeaders",
                 &WebRequestNS::SetResponseListener<kOnBeforeSendHeaders>)
      .SetMethod("onHeadersReceived",
                 &WebRequestNS::SetResponseListener<kOnHeadersReceived>)
      .SetMethod("onSendHeaders",
                 &WebRequestNS::SetSimpleListener<kOnSendHeaders>)
      .SetMethod("onBeforeRedirect",
                 &WebRequestNS::SetSimpleListener<kOnBeforeRedirect>)
      .SetMethod("onResponseStarted",
                 &WebRequestNS::SetSimpleListener<kOnResponseStarted>)
      .SetMethod("onErrorOccurred",
                 &WebRequestNS::SetSimpleListener<kOnErrorOccurred>)
      .SetMethod("onCompleted", &WebRequestNS::SetSimpleListener<kOnCompleted>);
}

const char* WebRequestNS::GetTypeName() {
  return "WebRequest";
}

int WebRequestNS::OnBeforeRequest(extensions::WebRequestInfo* info,
                                  const network::ResourceRequest& request,
                                  net::CompletionOnceCallback callback,
                                  GURL* new_url) {
  return HandleResponseEvent(kOnBeforeRequest, info, request,
                             std::move(callback), new_url);
}

int WebRequestNS::OnBeforeSendHeaders(extensions::WebRequestInfo* info,
                                      const network::ResourceRequest& request,
                                      BeforeSendHeadersCallback callback,
                                      net::HttpRequestHeaders* headers) {
  return HandleResponseEvent(
      kOnBeforeSendHeaders, info, request,
      base::BindOnce(std::move(callback), std::set<std::string>(),
                     std::set<std::string>()),
      headers, *headers);
}

int WebRequestNS::OnHeadersReceived(
    extensions::WebRequestInfo* info,
    const network::ResourceRequest& request,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return HandleResponseEvent(kOnHeadersReceived, info, request,
                             std::move(callback), original_response_headers,
                             override_response_headers,
                             allowed_unsafe_redirect_url);
}

void WebRequestNS::OnSendHeaders(extensions::WebRequestInfo* info,
                                 const network::ResourceRequest& request,
                                 const net::HttpRequestHeaders& headers) {
  HandleSimpleEvent(kOnSendHeaders, info, request, headers);
}

void WebRequestNS::OnBeforeRedirect(extensions::WebRequestInfo* info,
                                    const network::ResourceRequest& request,
                                    const GURL& new_location) {
  HandleSimpleEvent(kOnBeforeRedirect, info, request, new_location);
}

void WebRequestNS::OnResponseStarted(extensions::WebRequestInfo* info,
                                     const network::ResourceRequest& request) {
  HandleSimpleEvent(kOnResponseStarted, info, request);
}

void WebRequestNS::OnErrorOccurred(extensions::WebRequestInfo* info,
                                   const network::ResourceRequest& request,
                                   int net_error) {
  HandleSimpleEvent(kOnErrorOccurred, info, request, net_error);
}

void WebRequestNS::OnCompleted(extensions::WebRequestInfo* info,
                               const network::ResourceRequest& request,
                               int net_error) {
  HandleSimpleEvent(kOnCompleted, info, request, net_error);
}

template <WebRequestNS::SimpleEvent event>
void WebRequestNS::SetSimpleListener(gin::Arguments* args) {
  SetListener<SimpleListener>(event, &simple_listeners_, args);
}

template <WebRequestNS::ResponseEvent event>
void WebRequestNS::SetResponseListener(gin::Arguments* args) {
  SetListener<ResponseListener>(event, &response_listeners_, args);
}

template <typename Listener, typename Listeners, typename Event>
void WebRequestNS::SetListener(Event event,
                               Listeners* listeners,
                               gin::Arguments* args) {
  // { urls }.
  std::set<URLPattern> patterns;
  gin::Dictionary dict(args->isolate());
  args->GetNext(&dict) && dict.Get("urls", &patterns);

  // Function or null.
  v8::Local<v8::Value> value;
  Listener listener;
  if (!args->GetNext(&listener) &&
      !(args->GetNext(&value) && value->IsNull())) {
    args->ThrowTypeError("Must pass null or a Function");
    return;
  }

  if (listener.is_null())
    listeners->erase(event);
  else
    (*listeners)[event] = {std::move(patterns), std::move(listener)};
}

template <typename... Args>
void WebRequestNS::HandleSimpleEvent(SimpleEvent event,
                                     extensions::WebRequestInfo* info,
                                     const network::ResourceRequest& request,
                                     Args... args) {
  const auto& listener = simple_listeners_[event];
  if (!MatchesFilterCondition(info, listener.url_patterns))
    return;

  // TODO(zcbenz): Invoke the listener.
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
}

template <typename Out, typename... Args>
int WebRequestNS::HandleResponseEvent(ResponseEvent event,
                                      extensions::WebRequestInfo* info,
                                      const network::ResourceRequest& request,
                                      net::CompletionOnceCallback callback,
                                      Out out,
                                      Args... args) {
  const auto& listener = response_listeners_[event];
  if (!MatchesFilterCondition(info, listener.url_patterns))
    return net::OK;

  // TODO(zcbenz): Invoke the listener.
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  return net::OK;
}

// static
gin::Handle<WebRequestNS> WebRequestNS::FromOrCreate(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  gin::Handle<WebRequestNS> handle = From(isolate, browser_context);
  if (handle.IsEmpty()) {
    // Make sure the |Session| object has the |webRequest| property created.
    v8::Local<v8::Value> web_request =
        Session::CreateFrom(isolate,
                            static_cast<AtomBrowserContext*>(browser_context))
            ->WebRequest(isolate);
    gin::ConvertFromV8(isolate, web_request, &handle);
  }
  DCHECK(!handle.IsEmpty());
  return handle;
}

// static
gin::Handle<WebRequestNS> WebRequestNS::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  DCHECK(From(isolate, browser_context).IsEmpty())
      << "WebRequestNS already created";
  return gin::CreateHandle(isolate, new WebRequestNS(isolate, browser_context));
}

// static
gin::Handle<WebRequestNS> WebRequestNS::From(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  if (!browser_context)
    return gin::Handle<WebRequestNS>();
  auto* user_data =
      static_cast<UserData*>(browser_context->GetUserData(kUserDataKey));
  if (!user_data)
    return gin::Handle<WebRequestNS>();
  return gin::CreateHandle(isolate, user_data->data);
}

}  // namespace api

}  // namespace electron
