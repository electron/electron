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
#include "shell/browser/api/atom_api_web_contents.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/gin_converters/callback_converter_gin_adapter.h"
#include "shell/common/gin_converters/gurl_converter.h"
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

template <>
struct Converter<content::ResourceType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::ResourceType type) {
    const char* result;
    switch (type) {
      case content::ResourceType::kMainFrame:
        result = "mainFrame";
        break;
      case content::ResourceType::kSubFrame:
        result = "subFrame";
        break;
      case content::ResourceType::kStylesheet:
        result = "stylesheet";
        break;
      case content::ResourceType::kScript:
        result = "script";
        break;
      case content::ResourceType::kImage:
        result = "image";
        break;
      case content::ResourceType::kObject:
        result = "object";
        break;
      case content::ResourceType::kXhr:
        result = "xhr";
        break;
      default:
        result = "other";
    }
    return StringToV8(isolate, result);
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

// Overloaded by multiple types to fill the |details| object.
void ToDictionary(gin::Dictionary* details, extensions::WebRequestInfo* info) {
  details->Set("id", info->id);
  details->Set("url", info->url);
  details->Set("method", info->method);
  details->Set("timestamp", base::Time::Now().ToDoubleT() * 1000);
  if (!info->response_ip.empty())
    details->Set("ip", info->response_ip);
  if (info->type.has_value())
    details->Set("resourceType", info->type.value());
  else
    details->Set("resourceType", base::StringPiece("other"));
  if (info->response_headers) {
    details->Set("fromCache", info->response_from_cache);
    details->Set("responseHeaders", info->response_headers.get());
    details->Set("statusLine", info->response_headers->GetStatusLine());
    details->Set("statusCode", info->response_headers->response_code());
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(
      content::RenderFrameHost::FromID(info->render_process_id,
                                       info->frame_id));
  int32_t id = api::WebContents::GetIDFromWrappedClass(web_contents);
  // id must be greater than zero.
  if (id > 0)
    details->Set("webContentsId", id);
}

void ToDictionary(gin::Dictionary* details,
                  const network::ResourceRequest& request) {
  details->Set("referrer", request.referrer);
}

void ToDictionary(gin::Dictionary* details,
                  const net::HttpRequestHeaders& headers) {
  details->Set("requestHeaders", headers);
}

void ToDictionary(gin::Dictionary* details, const GURL& location) {
  details->Set("redirectURL", location);
}

void ToDictionary(gin::Dictionary* details, int net_error) {
  details->Set("error", net::ErrorToString(net_error));
}

// Helper function to fill |details| with arbitrary |args|.
template <typename Arg>
void FillDetails(gin::Dictionary* details, Arg arg) {
  ToDictionary(details, arg);
}

template <typename Arg, typename... Args>
void FillDetails(gin::Dictionary* details, Arg arg, Args... args) {
  ToDictionary(details, arg);
  FillDetails(details, args...);
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
  return HandleResponseEvent(kOnBeforeRequest, info, std::move(callback),
                             request, new_url);
}

int WebRequestNS::OnBeforeSendHeaders(extensions::WebRequestInfo* info,
                                      const network::ResourceRequest& request,
                                      BeforeSendHeadersCallback callback,
                                      net::HttpRequestHeaders* headers) {
  return HandleResponseEvent(
      kOnBeforeSendHeaders, info,
      base::BindOnce(std::move(callback), std::set<std::string>(),
                     std::set<std::string>()),
      request, headers, *headers);
}

int WebRequestNS::OnHeadersReceived(
    extensions::WebRequestInfo* info,
    const network::ResourceRequest& request,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return HandleResponseEvent(kOnHeadersReceived, info, std::move(callback),
                             request, original_response_headers,
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
                                     extensions::WebRequestInfo* request_info,
                                     Args... args) {
  const auto& info = simple_listeners_[event];
  if (!MatchesFilterCondition(request_info, info.url_patterns))
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  gin::Dictionary details(isolate, v8::Object::New(isolate));
  FillDetails(&details, request_info, args...);
  info.listener.Run(gin::ConvertToV8(isolate, details));
}

template <typename Out, typename... Args>
int WebRequestNS::HandleResponseEvent(ResponseEvent event,
                                      extensions::WebRequestInfo* request_info,
                                      net::CompletionOnceCallback callback,
                                      Out out,
                                      Args... args) {
  const auto& info = response_listeners_[event];
  if (!MatchesFilterCondition(request_info, info.url_patterns))
    return net::OK;

  // TODO(zcbenz): Invoke the listener.
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
