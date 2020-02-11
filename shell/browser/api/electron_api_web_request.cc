// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_request.h"

#include <memory>
#include <string>
#include <utility>

#include "base/stl_util.h"
#include "base/values.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_converters/value_converter.h"

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
struct Converter<blink::mojom::ResourceType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   blink::mojom::ResourceType type) {
    const char* result;
    switch (type) {
      case blink::mojom::ResourceType::kMainFrame:
        result = "mainFrame";
        break;
      case blink::mojom::ResourceType::kSubFrame:
        result = "subFrame";
        break;
      case blink::mojom::ResourceType::kStylesheet:
        result = "stylesheet";
        break;
      case blink::mojom::ResourceType::kScript:
        result = "script";
        break;
      case blink::mojom::ResourceType::kImage:
        result = "image";
        break;
      case blink::mojom::ResourceType::kObject:
        result = "object";
        break;
      case blink::mojom::ResourceType::kXhr:
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

const char* kUserDataKey = "WebRequest";

// BrowserContext <=> WebRequest relationship.
struct UserData : public base::SupportsUserData::Data {
  explicit UserData(WebRequest* data) : data(data) {}
  WebRequest* data;
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

// Convert HttpResponseHeaders to V8.
//
// Note that while we already have converters for HttpResponseHeaders, we can
// not use it because it lowercases the header keys, while the webRequest has
// to pass the original keys.
v8::Local<v8::Value> HttpResponseHeadersToV8(
    net::HttpResponseHeaders* headers) {
  base::DictionaryValue response_headers;
  if (headers) {
    size_t iter = 0;
    std::string key;
    std::string value;
    while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
      base::Value* values = response_headers.FindListKey(key);
      if (!values)
        values = response_headers.SetKey(key, base::ListValue());
      values->Append(value);
    }
  }
  return gin::ConvertToV8(v8::Isolate::GetCurrent(), response_headers);
}

// Overloaded by multiple types to fill the |details| object.
void ToDictionary(gin::Dictionary* details, extensions::WebRequestInfo* info) {
  details->Set("id", info->id);
  details->Set("url", info->url);
  details->Set("method", info->method);
  details->Set("timestamp", base::Time::Now().ToDoubleT() * 1000);
  details->Set("resourceType", info->type);
  if (!info->response_ip.empty())
    details->Set("ip", info->response_ip);
  if (info->response_headers) {
    details->Set("fromCache", info->response_from_cache);
    details->Set("statusLine", info->response_headers->GetStatusLine());
    details->Set("statusCode", info->response_headers->response_code());
    details->Set("responseHeaders",
                 HttpResponseHeadersToV8(info->response_headers.get()));
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
  if (request.request_body)
    details->Set("uploadData", *request.request_body);
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

// Fill the native types with the result from the response object.
void ReadFromResponse(v8::Isolate* isolate,
                      gin::Dictionary* response,
                      GURL* new_location) {
  response->Get("redirectURL", new_location);
}

void ReadFromResponse(v8::Isolate* isolate,
                      gin::Dictionary* response,
                      net::HttpRequestHeaders* headers) {
  headers->Clear();
  response->Get("requestHeaders", headers);
}

void ReadFromResponse(v8::Isolate* isolate,
                      gin::Dictionary* response,
                      const std::pair<scoped_refptr<net::HttpResponseHeaders>*,
                                      const std::string>& headers) {
  std::string status_line;
  if (!response->Get("statusLine", &status_line))
    status_line = headers.second;
  v8::Local<v8::Value> value;
  if (response->Get("responseHeaders", &value) && value->IsObject()) {
    *headers.first = new net::HttpResponseHeaders("");
    (*headers.first)->ReplaceStatusLine(status_line);
    gin::Converter<net::HttpResponseHeaders*>::FromV8(isolate, value,
                                                      (*headers.first).get());
  }
}

}  // namespace

gin::WrapperInfo WebRequest::kWrapperInfo = {gin::kEmbedderNativeGin};

WebRequest::SimpleListenerInfo::SimpleListenerInfo(
    std::set<URLPattern> patterns_,
    SimpleListener listener_)
    : url_patterns(std::move(patterns_)), listener(listener_) {}
WebRequest::SimpleListenerInfo::SimpleListenerInfo() = default;
WebRequest::SimpleListenerInfo::~SimpleListenerInfo() = default;

WebRequest::ResponseListenerInfo::ResponseListenerInfo(
    std::set<URLPattern> patterns_,
    ResponseListener listener_)
    : url_patterns(std::move(patterns_)), listener(listener_) {}
WebRequest::ResponseListenerInfo::ResponseListenerInfo() = default;
WebRequest::ResponseListenerInfo::~ResponseListenerInfo() = default;

WebRequest::WebRequest(v8::Isolate* isolate,
                       content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  browser_context_->SetUserData(kUserDataKey, std::make_unique<UserData>(this));
}

WebRequest::~WebRequest() {
  browser_context_->RemoveUserData(kUserDataKey);
}

gin::ObjectTemplateBuilder WebRequest::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebRequest>::GetObjectTemplateBuilder(isolate)
      .SetMethod("onBeforeRequest",
                 &WebRequest::SetResponseListener<kOnBeforeRequest>)
      .SetMethod("onBeforeSendHeaders",
                 &WebRequest::SetResponseListener<kOnBeforeSendHeaders>)
      .SetMethod("onHeadersReceived",
                 &WebRequest::SetResponseListener<kOnHeadersReceived>)
      .SetMethod("onSendHeaders",
                 &WebRequest::SetSimpleListener<kOnSendHeaders>)
      .SetMethod("onBeforeRedirect",
                 &WebRequest::SetSimpleListener<kOnBeforeRedirect>)
      .SetMethod("onResponseStarted",
                 &WebRequest::SetSimpleListener<kOnResponseStarted>)
      .SetMethod("onErrorOccurred",
                 &WebRequest::SetSimpleListener<kOnErrorOccurred>)
      .SetMethod("onCompleted", &WebRequest::SetSimpleListener<kOnCompleted>);
}

const char* WebRequest::GetTypeName() {
  return "WebRequest";
}

bool WebRequest::HasListener() const {
  return !(simple_listeners_.empty() && response_listeners_.empty());
}

int WebRequest::OnBeforeRequest(extensions::WebRequestInfo* info,
                                const network::ResourceRequest& request,
                                net::CompletionOnceCallback callback,
                                GURL* new_url) {
  return HandleResponseEvent(kOnBeforeRequest, info, std::move(callback),
                             new_url, request);
}

int WebRequest::OnBeforeSendHeaders(extensions::WebRequestInfo* info,
                                    const network::ResourceRequest& request,
                                    BeforeSendHeadersCallback callback,
                                    net::HttpRequestHeaders* headers) {
  return HandleResponseEvent(
      kOnBeforeSendHeaders, info,
      base::BindOnce(std::move(callback), std::set<std::string>(),
                     std::set<std::string>()),
      headers, request, *headers);
}

int WebRequest::OnHeadersReceived(
    extensions::WebRequestInfo* info,
    const network::ResourceRequest& request,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return HandleResponseEvent(
      kOnHeadersReceived, info, std::move(callback),
      std::make_pair(override_response_headers,
                     original_response_headers->GetStatusLine()),
      request);
}

void WebRequest::OnSendHeaders(extensions::WebRequestInfo* info,
                               const network::ResourceRequest& request,
                               const net::HttpRequestHeaders& headers) {
  HandleSimpleEvent(kOnSendHeaders, info, request, headers);
}

void WebRequest::OnBeforeRedirect(extensions::WebRequestInfo* info,
                                  const network::ResourceRequest& request,
                                  const GURL& new_location) {
  HandleSimpleEvent(kOnBeforeRedirect, info, request, new_location);
}

void WebRequest::OnResponseStarted(extensions::WebRequestInfo* info,
                                   const network::ResourceRequest& request) {
  HandleSimpleEvent(kOnResponseStarted, info, request);
}

void WebRequest::OnErrorOccurred(extensions::WebRequestInfo* info,
                                 const network::ResourceRequest& request,
                                 int net_error) {
  callbacks_.erase(info->id);

  HandleSimpleEvent(kOnErrorOccurred, info, request, net_error);
}

void WebRequest::OnCompleted(extensions::WebRequestInfo* info,
                             const network::ResourceRequest& request,
                             int net_error) {
  callbacks_.erase(info->id);

  HandleSimpleEvent(kOnCompleted, info, request, net_error);
}

void WebRequest::OnRequestWillBeDestroyed(extensions::WebRequestInfo* info) {
  callbacks_.erase(info->id);
}

template <WebRequest::SimpleEvent event>
void WebRequest::SetSimpleListener(gin::Arguments* args) {
  SetListener<SimpleListener>(event, &simple_listeners_, args);
}

template <WebRequest::ResponseEvent event>
void WebRequest::SetResponseListener(gin::Arguments* args) {
  SetListener<ResponseListener>(event, &response_listeners_, args);
}

template <typename Listener, typename Listeners, typename Event>
void WebRequest::SetListener(Event event,
                             Listeners* listeners,
                             gin::Arguments* args) {
  v8::Local<v8::Value> arg;

  // { urls }.
  std::set<std::string> filter_patterns;
  gin::Dictionary dict(args->isolate());
  if (args->GetNext(&arg) && !arg->IsFunction()) {
    // Note that gin treats Function as Dictionary when doing convertions, so we
    // have to explicitly check if the argument is Function before trying to
    // convert it to Dictionary.
    if (gin::ConvertFromV8(args->isolate(), arg, &dict)) {
      if (!dict.Get("urls", &filter_patterns)) {
        args->ThrowTypeError("Parameter 'filter' must have property 'urls'.");
        return;
      }
      args->GetNext(&arg);
    }
  }

  std::set<URLPattern> patterns;
  for (const std::string& filter_pattern : filter_patterns) {
    URLPattern pattern(URLPattern::SCHEME_ALL);
    const URLPattern::ParseResult result = pattern.Parse(filter_pattern);
    if (result == URLPattern::ParseResult::kSuccess) {
      patterns.insert(pattern);
    } else {
      const char* error_type = URLPattern::GetParseResultString(result);
      args->ThrowTypeError("Invalid url pattern " + filter_pattern + ": " +
                           error_type);
      return;
    }
  }

  // Function or null.
  Listener listener;
  if (arg.IsEmpty() ||
      !(gin::ConvertFromV8(args->isolate(), arg, &listener) || arg->IsNull())) {
    args->ThrowTypeError("Must pass null or a Function");
    return;
  }

  if (listener.is_null())
    listeners->erase(event);
  else
    (*listeners)[event] = {std::move(patterns), std::move(listener)};
}

template <typename... Args>
void WebRequest::HandleSimpleEvent(SimpleEvent event,
                                   extensions::WebRequestInfo* request_info,
                                   Args... args) {
  const auto iter = simple_listeners_.find(event);
  if (iter == std::end(simple_listeners_))
    return;

  const auto& info = iter->second;
  if (!MatchesFilterCondition(request_info, info.url_patterns))
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  gin::Dictionary details(isolate, v8::Object::New(isolate));
  FillDetails(&details, request_info, args...);
  info.listener.Run(gin::ConvertToV8(isolate, details));
}

template <typename Out, typename... Args>
int WebRequest::HandleResponseEvent(ResponseEvent event,
                                    extensions::WebRequestInfo* request_info,
                                    net::CompletionOnceCallback callback,
                                    Out out,
                                    Args... args) {
  const auto iter = response_listeners_.find(event);
  if (iter == std::end(response_listeners_))
    return net::OK;

  const auto& info = iter->second;
  if (!MatchesFilterCondition(request_info, info.url_patterns))
    return net::OK;

  callbacks_[request_info->id] = std::move(callback);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  gin::Dictionary details(isolate, v8::Object::New(isolate));
  FillDetails(&details, request_info, args...);

  ResponseCallback response =
      base::BindOnce(&WebRequest::OnListenerResult<Out>, base::Unretained(this),
                     request_info->id, out);
  info.listener.Run(gin::ConvertToV8(isolate, details), std::move(response));
  return net::ERR_IO_PENDING;
}

template <typename T>
void WebRequest::OnListenerResult(uint64_t id,
                                  T out,
                                  v8::Local<v8::Value> response) {
  const auto iter = callbacks_.find(id);
  if (iter == std::end(callbacks_))
    return;

  int result = net::OK;
  if (response->IsObject()) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    gin::Dictionary dict(isolate, response.As<v8::Object>());

    bool cancel = false;
    dict.Get("cancel", &cancel);
    if (cancel)
      result = net::ERR_BLOCKED_BY_CLIENT;
    else
      ReadFromResponse(isolate, &dict, out);
  }

  // The ProxyingURLLoaderFactory expects the callback to be executed
  // asynchronously, because it used to work on IO thread before NetworkService.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callbacks_[id]), result));
  callbacks_.erase(iter);
}

// static
gin::Handle<WebRequest> WebRequest::FromOrCreate(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  gin::Handle<WebRequest> handle = From(isolate, browser_context);
  if (handle.IsEmpty()) {
    // Make sure the |Session| object has the |webRequest| property created.
    v8::Local<v8::Value> web_request =
        Session::CreateFrom(
            isolate, static_cast<ElectronBrowserContext*>(browser_context))
            ->WebRequest(isolate);
    gin::ConvertFromV8(isolate, web_request, &handle);
  }
  DCHECK(!handle.IsEmpty());
  return handle;
}

// static
gin::Handle<WebRequest> WebRequest::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  DCHECK(From(isolate, browser_context).IsEmpty())
      << "WebRequest already created";
  return gin::CreateHandle(isolate, new WebRequest(isolate, browser_context));
}

// static
gin::Handle<WebRequest> WebRequest::From(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  if (!browser_context)
    return gin::Handle<WebRequest>();
  auto* user_data =
      static_cast<UserData*>(browser_context->GetUserData(kUserDataKey));
  if (!user_data)
    return gin::Handle<WebRequest>();
  return gin::CreateHandle(isolate, user_data->data);
}

}  // namespace api

}  // namespace electron
