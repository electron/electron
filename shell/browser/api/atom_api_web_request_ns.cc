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
#include "shell/browser/atom_browser_context.h"
#include "shell/common/gin_converters/callback_converter_gin_adapter.h"
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
bool MatchesFilterCondition(const network::ResourceRequest& request,
                            const std::set<URLPattern>& patterns) {
  if (patterns.empty())
    return true;

  for (const auto& pattern : patterns) {
    if (pattern.MatchesURL(request.url))
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
                           content::BrowserContext* browser_context) {
  browser_context->SetUserData(kUserDataKey, std::make_unique<UserData>(this));
}

WebRequestNS::~WebRequestNS() = default;

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

int WebRequestNS::OnBeforeRequest(const network::ResourceRequest& request,
                                  net::CompletionOnceCallback callback,
                                  GURL* new_url) {
  return HandleResponseEvent(kOnBeforeRequest, request, std::move(callback),
                             new_url);
}

int WebRequestNS::OnBeforeSendHeaders(const network::ResourceRequest& request,
                                      BeforeSendHeadersCallback callback,
                                      net::HttpRequestHeaders* headers) {
  // TODO(zcbenz): Figure out how to handle this generally.
  return net::OK;
}

int WebRequestNS::OnHeadersReceived(
    const network::ResourceRequest& request,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return HandleResponseEvent(kOnHeadersReceived, request, std::move(callback),
                             original_response_headers,
                             override_response_headers,
                             allowed_unsafe_redirect_url);
}

void WebRequestNS::OnSendHeaders(const network::ResourceRequest& request,
                                 const net::HttpRequestHeaders& headers) {
  HandleSimpleEvent(kOnSendHeaders, request, headers);
}

void WebRequestNS::OnBeforeRedirect(
    const network::ResourceRequest& request,
    const network::ResourceResponseHead& response,
    const GURL& new_location) {
  HandleSimpleEvent(kOnBeforeRedirect, request, response, new_location);
}

void WebRequestNS::OnResponseStarted(
    const network::ResourceRequest& request,
    const network::ResourceResponseHead& response) {
  HandleSimpleEvent(kOnResponseStarted, request, response);
}

void WebRequestNS::OnErrorOccurred(
    const network::ResourceRequest& request,
    const network::ResourceResponseHead& response,
    int net_error) {
  HandleSimpleEvent(kOnErrorOccurred, request, response, net_error);
}

void WebRequestNS::OnCompleted(const network::ResourceRequest& request,
                               const network::ResourceResponseHead& response,
                               int net_error) {
  HandleSimpleEvent(kOnCompleted, request, response, net_error);
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
                                     const network::ResourceRequest& request,
                                     Args... args) {
  const auto& info = simple_listeners_[event];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return;

  // TODO(zcbenz): Invoke the listener.
}

template <typename Out, typename... Args>
int WebRequestNS::HandleResponseEvent(ResponseEvent event,
                                      const network::ResourceRequest& request,
                                      net::CompletionOnceCallback callback,
                                      Out out,
                                      Args... args) {
  const auto& info = response_listeners_[event];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return net::OK;

  // TODO(zcbenz): Invoke the listener.
  return net::OK;
}

// static
gin::Handle<WebRequestNS> WebRequestNS::FromOrCreate(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  auto* web_request = From(browser_context);
  if (!web_request)
    web_request = new WebRequestNS(isolate, browser_context);
  return gin::CreateHandle(isolate, web_request);
}

// static
WebRequestNS* WebRequestNS::From(content::BrowserContext* browser_context) {
  if (!browser_context)
    return nullptr;
  auto* user_data =
      static_cast<UserData*>(browser_context->GetUserData(kUserDataKey));
  if (!user_data)
    return nullptr;
  return user_data->data;
}

}  // namespace api

}  // namespace electron
