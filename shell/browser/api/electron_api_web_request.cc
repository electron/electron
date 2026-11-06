// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_request.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_utils.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "extensions/common/api/web_request/web_request_resource_type.h"
#include "extensions/common/url_pattern.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "gin/persistent.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/login_handler.h"
#include "shell/browser/net/proxying_url_loader_factory.h"
#include "shell/browser/net/url_loader_factory_gate.h"
#include "shell/common/api/electron_api_url_loader.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"

static constexpr auto ResourceTypes =
    base::MakeFixedFlatMap<std::string_view,
                           extensions::WebRequestResourceType>({
        {"cspReport", extensions::WebRequestResourceType::CSP_REPORT},
        {"font", extensions::WebRequestResourceType::FONT},
        {"image", extensions::WebRequestResourceType::IMAGE},
        {"mainFrame", extensions::WebRequestResourceType::MAIN_FRAME},
        {"media", extensions::WebRequestResourceType::MEDIA},
        {"object", extensions::WebRequestResourceType::OBJECT},
        {"ping", extensions::WebRequestResourceType::PING},
        {"script", extensions::WebRequestResourceType::SCRIPT},
        {"stylesheet", extensions::WebRequestResourceType::STYLESHEET},
        {"subFrame", extensions::WebRequestResourceType::SUB_FRAME},
        {"webSocket", extensions::WebRequestResourceType::WEB_SOCKET},
        {"xhr", extensions::WebRequestResourceType::XHR},
    });

namespace gin {

template <>
struct Converter<extensions::WebRequestResourceType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   extensions::WebRequestResourceType type) {
    for (const auto& [name, val] : ResourceTypes)
      if (type == val)
        return StringToV8(isolate, name);

    return StringToV8(isolate, "other");
  }
};

}  // namespace gin

namespace electron::api {

namespace {

extensions::WebRequestResourceType ParseResourceType(std::string_view value) {
  if (auto iter = ResourceTypes.find(value); iter != ResourceTypes.end())
    return iter->second;

  return extensions::WebRequestResourceType::OTHER;
}

// Convert HttpResponseHeaders to V8.
//
// Note that while we already have converters for HttpResponseHeaders, we can
// not use it because it lowercases the header keys, while the webRequest has
// to pass the original keys.
v8::Local<v8::Value> HttpResponseHeadersToV8(
    net::HttpResponseHeaders* headers) {
  base::DictValue response_headers;
  if (headers) {
    size_t iter = 0;
    std::string key;
    std::string value;
    while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
      response_headers.EnsureList(key)->Append(value);
    }
  }
  return gin::ConvertToV8(v8::Isolate::GetCurrent(), response_headers);
}

// Overloaded by multiple types to fill the |details| object.
void ToDictionary(gin_helper::Dictionary* details,
                  const extensions::WebRequestInfo* info) {
  details->Set("id", info->id);
  details->Set("url", info->url);
  details->Set("method", info->method);
  details->Set("timestamp",
               base::Time::Now().InSecondsFSinceUnixEpoch() * 1000);
  details->Set("resourceType", info->web_request_type);
  if (!info->response_ip.empty())
    details->Set("ip", info->response_ip);
  // The origin that issued the request (Chrome's webRequest `initiator`). It
  // is kept from the original request across redirects and, unlike
  // `referrer`, is not under the requesting document's control.
  if (info->initiator)
    details->Set("initiatorOrigin", info->initiator->Serialize());
  if (info->response_headers) {
    details->Set("fromCache", info->response_from_cache);
    details->Set("statusLine", info->response_headers->GetStatusLine());
    details->Set("statusCode", info->response_headers->response_code());
    details->Set("responseHeaders",
                 HttpResponseHeadersToV8(info->response_headers.get()));
  }

  auto* render_frame_host = content::RenderFrameHost::FromID(info->global_id);
  if (render_frame_host) {
    details->SetGetter("frame", render_frame_host);
    auto* web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    auto* api_web_contents = WebContents::From(web_contents);
    if (api_web_contents) {
      details->Set("webContents", api_web_contents);
      details->Set("webContentsId", api_web_contents->ID());
    }
  }
}

void ToDictionary(gin_helper::Dictionary* details,
                  const network::ResourceRequest& request) {
  details->Set("referrer", request.referrer);
}

void ToDictionary(gin_helper::Dictionary* details,
                  const net::HttpRequestHeaders& headers) {
  details->Set("requestHeaders", headers);
}

void ToDictionary(gin_helper::Dictionary* details, const GURL& location) {
  details->Set("redirectURL", location);
}

void ToDictionary(gin_helper::Dictionary* details, int net_error) {
  details->Set("error", net::ErrorToString(net_error));
}

// Helper function to fill |details| with arbitrary |args|.
template <typename Arg>
void FillDetails(gin_helper::Dictionary* details, Arg arg) {
  ToDictionary(details, arg);
}

template <typename Arg, typename... Args>
void FillDetails(gin_helper::Dictionary* details, Arg arg, Args... args) {
  ToDictionary(details, arg);
  FillDetails(details, args...);
}

// Modified from extensions/browser/api/web_request/web_request_api_helpers.cc.
std::pair<std::set<std::string>, std::set<std::string>>
CalculateOnBeforeSendHeadersDelta(const net::HttpRequestHeaders* old_headers,
                                  const net::HttpRequestHeaders* new_headers) {
  // Newly introduced or overridden request headers.
  std::set<std::string> modified_request_headers;
  // Keys of request headers to be deleted.
  std::set<std::string> deleted_request_headers;

  // The event listener might not have passed any new headers if it
  // just wanted to cancel the request.
  if (new_headers) {
    // Find deleted headers.
    {
      net::HttpRequestHeaders::Iterator i(*old_headers);
      while (i.GetNext()) {
        if (!new_headers->HasHeader(i.name())) {
          deleted_request_headers.insert(i.name());
        }
      }
    }

    // Find modified headers.
    {
      net::HttpRequestHeaders::Iterator i(*new_headers);
      while (i.GetNext()) {
        if (i.value() != old_headers->GetHeader(i.name())) {
          modified_request_headers.insert(i.name());
        }
      }
    }
  }

  return std::make_pair(modified_request_headers, deleted_request_headers);
}

WebRequest* ForObservedRequest(
    const base::WeakPtr<ElectronBrowserContext>& browser_context) {
  if (!browser_context)
    return nullptr;
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  return WebRequest::FromOrCreate(isolate, browser_context.get());
}

}  // namespace

const gin::WrapperInfo WebRequest::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronWebRequest);

WebRequest::RequestFilter::RequestFilter(const RequestFilter&) = default;
WebRequest::RequestFilter::RequestFilter(RequestFilter&&) = default;
WebRequest::RequestFilter& WebRequest::RequestFilter::operator=(
    const RequestFilter&) = default;
WebRequest::RequestFilter& WebRequest::RequestFilter::operator=(
    RequestFilter&&) = default;
WebRequest::RequestFilter::RequestFilter() = default;
WebRequest::RequestFilter::~RequestFilter() = default;

void WebRequest::RequestFilter::AddUrlPattern(URLPattern pattern,
                                              bool is_match_pattern) {
  if (is_match_pattern) {
    include_url_patterns_.emplace(std::move(pattern));
  } else {
    exclude_url_patterns_.emplace(std::move(pattern));
  }
}

void WebRequest::RequestFilter::AddType(
    extensions::WebRequestResourceType type) {
  types_.insert(type);
}

bool WebRequest::RequestFilter::MatchesURL(
    const GURL& url,
    const std::set<URLPattern>& patterns) const {
  if (patterns.empty())
    return false;

  for (const auto& pattern : patterns) {
    if (pattern.MatchesURL(url))
      return true;
  }
  return false;
}

bool WebRequest::RequestFilter::MatchesType(
    extensions::WebRequestResourceType type) const {
  return types_.empty() || types_.contains(type);
}

bool WebRequest::RequestFilter::MatchesRequest(
    const extensions::WebRequestInfo* info) const {
  // Matches URL and type, and does not match exclude URL.
  return MatchesURL(info->url, include_url_patterns_) &&
         !MatchesURL(info->url, exclude_url_patterns_) &&
         MatchesType(info->web_request_type);
}

uint32_t WebRequest::RequestFilter::TypeMask() const {
  if (types_.empty())
    return kAllResourceTypes;
  uint32_t mask = 0;
  for (auto type : types_)
    mask |= 1u << static_cast<int>(type);
  return mask;
}

void WebRequest::RequestFilter::AddUrlPatterns(
    const std::set<std::string>& filter_patterns,
    RequestFilter* filter,
    gin::Arguments* args,
    bool is_match_pattern) {
  for (const std::string& filter_pattern : filter_patterns) {
    URLPattern pattern(URLPattern::SCHEME_ALL);
    const URLPattern::ParseResult result = pattern.Parse(filter_pattern);
    if (result == URLPattern::ParseResult::kSuccess) {
      filter->AddUrlPattern(std::move(pattern), is_match_pattern);
    } else {
      const char* error_type = URLPattern::GetParseResultString(result);
      args->ThrowTypeError("Invalid url pattern " + filter_pattern + ": " +
                           error_type);
      return;
    }
  }
}

struct WebRequest::BlockedRequest {
  BlockedRequest() = default;
  raw_ptr<const extensions::WebRequestInfo> request = nullptr;
  net::CompletionOnceCallback callback;
  // Only used for onBeforeSendHeaders.
  BeforeSendHeadersCallback before_send_headers_callback;
  // The callback to invoke for auth. If |auth_callback.is_null()| is false,
  // |callback| must be NULL.
  // Only valid for OnAuthRequired.
  AuthCallback auth_callback;
  // Only used for onBeforeSendHeaders.
  raw_ptr<net::HttpRequestHeaders> request_headers = nullptr;
  // Only used for onHeadersReceived.
  scoped_refptr<const net::HttpResponseHeaders> original_response_headers;
  // Only used for onHeadersReceived.
  raw_ptr<scoped_refptr<net::HttpResponseHeaders>> override_response_headers =
      nullptr;
  std::string status_line;
  // Only used for onBeforeRequest.
  raw_ptr<GURL> new_url = nullptr;
  // Owns the LoginHandler while waiting for auth credentials.
  std::unique_ptr<LoginHandler> login_handler;
};

WebRequest::SimpleListenerInfo::SimpleListenerInfo(RequestFilter filter_,
                                                   SimpleListener listener_)
    : filter(std::move(filter_)), listener(listener_) {}
WebRequest::SimpleListenerInfo::SimpleListenerInfo() = default;
WebRequest::SimpleListenerInfo::~SimpleListenerInfo() = default;

WebRequest::ResponseListenerInfo::ResponseListenerInfo(
    RequestFilter filter_,
    ResponseListener listener_)
    : filter(std::move(filter_)), listener(listener_) {}
WebRequest::ResponseListenerInfo::ResponseListenerInfo() = default;
WebRequest::ResponseListenerInfo::~ResponseListenerInfo() = default;

gin::ObjectTemplateBuilder WebRequest::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebRequest>::GetObjectTemplateBuilder(isolate)
      .SetMethod(
          "onBeforeRequest",
          &WebRequest::SetResponseListener<ResponseEvent::kOnBeforeRequest>)
      .SetMethod(
          "onBeforeSendHeaders",
          &WebRequest::SetResponseListener<ResponseEvent::kOnBeforeSendHeaders>)
      .SetMethod(
          "onHeadersReceived",
          &WebRequest::SetResponseListener<ResponseEvent::kOnHeadersReceived>)
      .SetMethod("onSendHeaders",
                 &WebRequest::SetSimpleListener<SimpleEvent::kOnSendHeaders>)
      .SetMethod("onBeforeRedirect",
                 &WebRequest::SetSimpleListener<SimpleEvent::kOnBeforeRedirect>)
      .SetMethod(
          "onResponseStarted",
          &WebRequest::SetSimpleListener<SimpleEvent::kOnResponseStarted>)
      .SetMethod("onErrorOccurred",
                 &WebRequest::SetSimpleListener<SimpleEvent::kOnErrorOccurred>)
      .SetMethod("onCompleted",
                 &WebRequest::SetSimpleListener<SimpleEvent::kOnCompleted>)
      .SetMethod("_startFileFetch", &WebRequest::StartFileFetch)
      .SetMethod("_fileFetchBeforeRequest", &WebRequest::FileFetchBeforeRequest)
      .SetMethod("_fileFetchResponded", &WebRequest::FileFetchResponded)
      .SetMethod("_abortFileFetch", &WebRequest::AbortFileFetch);
}

const gin::WrapperInfo* WebRequest::wrapper_info() const {
  return &kWrapperInfo;
}

const char* WebRequest::GetHumanReadableName() const {
  return "Electron / WebRequest";
}

void WebRequest::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<WebRequest>::Trace(visitor);
  visitor->Trace(weak_factory_);
}

bool WebRequest::HasListener() const {
  return !(simple_listeners_.empty() && response_listeners_.empty());
}

int WebRequest::OnBeforeRequest(extensions::WebRequestInfo* info,
                                const network::ResourceRequest& request,
                                net::CompletionOnceCallback callback,
                                GURL* new_url) {
  return HandleOnBeforeRequestResponseEvent(info, request, std::move(callback),
                                            new_url);
}

int WebRequest::HandleOnBeforeRequestResponseEvent(
    extensions::WebRequestInfo* request_info,
    const network::ResourceRequest& request,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  const auto iter = response_listeners_.find(ResponseEvent::kOnBeforeRequest);
  if (iter == std::end(response_listeners_))
    return net::OK;

  const auto& info = iter->second;
  if (!info.filter.MatchesRequest(request_info))
    return net::OK;

  BlockedRequest blocked_request;
  blocked_request.callback = std::move(callback);
  blocked_request.new_url = new_url;
  blocked_requests_[request_info->id] = std::move(blocked_request);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary details(isolate, v8::Object::New(isolate));
  FillDetails(&details, request_info, request, *new_url);
  if (request.request_body)
    details.Set("uploadData", *request.request_body);

  auto& allocation_handle = isolate->GetCppHeap()->GetAllocationHandle();
  ResponseCallback response = base::BindOnce(
      &WebRequest::OnBeforeRequestListenerResult,
      gin::WrapPersistent(weak_factory_.GetWeakCell(allocation_handle)),
      request_info->id);
  info.listener.Run(gin::ConvertToV8(isolate, details), std::move(response));
  return net::ERR_IO_PENDING;
}

void WebRequest::OnBeforeRequestListenerResult(uint64_t id,
                                               v8::Local<v8::Value> response) {
  auto nh = blocked_requests_.extract(id);
  if (!nh)
    return;

  auto& request = nh.mapped();

  int result = net::OK;
  if (response->IsObject()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    gin::Dictionary dict(isolate, response.As<v8::Object>());

    bool cancel = false;
    dict.Get("cancel", &cancel);
    if (cancel) {
      result = net::ERR_BLOCKED_BY_CLIENT;
    } else {
      dict.Get("redirectURL", request.new_url.get());
    }
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(request.callback), result));
}

int WebRequest::OnBeforeSendHeaders(extensions::WebRequestInfo* info,
                                    const network::ResourceRequest& request,
                                    BeforeSendHeadersCallback callback,
                                    net::HttpRequestHeaders* headers) {
  return HandleOnBeforeSendHeadersResponseEvent(info, request,
                                                std::move(callback), headers);
}

int WebRequest::HandleOnBeforeSendHeadersResponseEvent(
    extensions::WebRequestInfo* request_info,
    const network::ResourceRequest& request,
    BeforeSendHeadersCallback callback,
    net::HttpRequestHeaders* headers) {
  const auto iter =
      response_listeners_.find(ResponseEvent::kOnBeforeSendHeaders);
  if (iter == std::end(response_listeners_))
    return net::OK;

  const auto& info = iter->second;
  if (!info.filter.MatchesRequest(request_info))
    return net::OK;

  BlockedRequest blocked_request;
  blocked_request.before_send_headers_callback = std::move(callback);
  blocked_request.request_headers = headers;
  blocked_requests_[request_info->id] = std::move(blocked_request);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary details(isolate, v8::Object::New(isolate));
  FillDetails(&details, request_info, request, *headers);
  if (request.request_body)
    details.Set("uploadData", *request.request_body);

  ResponseCallback response =
      base::BindOnce(&WebRequest::OnBeforeSendHeadersListenerResult,
                     base::Unretained(this), request_info->id);
  info.listener.Run(gin::ConvertToV8(isolate, details), std::move(response));
  return net::ERR_IO_PENDING;
}

void WebRequest::OnBeforeSendHeadersListenerResult(
    uint64_t id,
    v8::Local<v8::Value> response) {
  auto nh = blocked_requests_.extract(id);
  if (!nh)
    return;

  auto& request = nh.mapped();

  net::HttpRequestHeaders* old_headers = request.request_headers;
  net::HttpRequestHeaders new_headers;

  int result = net::OK;
  bool user_modified_headers = false;
  if (response->IsObject()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    gin::Dictionary dict(isolate, response.As<v8::Object>());

    bool cancel = false;
    dict.Get("cancel", &cancel);
    if (cancel) {
      result = net::ERR_BLOCKED_BY_CLIENT;
    } else {
      v8::Local<v8::Value> value;
      if (dict.Get("requestHeaders", &value) && value->IsObject()) {
        user_modified_headers = true;
        gin::Converter<net::HttpRequestHeaders>::FromV8(isolate, value,
                                                        &new_headers);
      }
    }
  }

  // If the user passes |cancel|, |new_headers| should be nullptr.
  const auto updated_headers = CalculateOnBeforeSendHeadersDelta(
      old_headers,
      result == net::ERR_BLOCKED_BY_CLIENT ? nullptr : &new_headers);

  // Leave |request.request_headers| unchanged if the user didn't modify it.
  if (user_modified_headers)
    request.request_headers->Swap(&new_headers);

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(request.before_send_headers_callback),
                     updated_headers.first, updated_headers.second, result));
}

int WebRequest::OnHeadersReceived(
    extensions::WebRequestInfo* info,
    const network::ResourceRequest& request,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return HandleOnHeadersReceivedResponseEvent(
      info, request, std::move(callback), original_response_headers,
      override_response_headers);
}

int WebRequest::HandleOnHeadersReceivedResponseEvent(
    extensions::WebRequestInfo* request_info,
    const network::ResourceRequest& request,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers) {
  const auto iter = response_listeners_.find(ResponseEvent::kOnHeadersReceived);
  if (iter == std::end(response_listeners_))
    return net::OK;

  const auto& info = iter->second;
  if (!info.filter.MatchesRequest(request_info))
    return net::OK;

  BlockedRequest blocked_request;
  blocked_request.callback = std::move(callback);
  blocked_request.override_response_headers = override_response_headers;
  blocked_request.status_line = original_response_headers
                                    ? original_response_headers->GetStatusLine()
                                    : std::string();
  blocked_requests_[request_info->id] = std::move(blocked_request);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary details(isolate, v8::Object::New(isolate));
  FillDetails(&details, request_info, request);

  ResponseCallback response =
      base::BindOnce(&WebRequest::OnHeadersReceivedListenerResult,
                     base::Unretained(this), request_info->id);
  info.listener.Run(gin::ConvertToV8(isolate, details), std::move(response));
  return net::ERR_IO_PENDING;
}

void WebRequest::OnHeadersReceivedListenerResult(
    uint64_t id,
    v8::Local<v8::Value> response) {
  auto nh = blocked_requests_.extract(id);
  if (!nh)
    return;

  auto& request = nh.mapped();

  int result = net::OK;
  bool user_modified_headers = false;
  scoped_refptr<net::HttpResponseHeaders> override_headers(
      new net::HttpResponseHeaders(""));
  if (response->IsObject()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    gin::Dictionary dict(isolate, response.As<v8::Object>());

    bool cancel = false;
    dict.Get("cancel", &cancel);
    if (cancel) {
      result = net::ERR_BLOCKED_BY_CLIENT;
    } else {
      std::string status_line;
      if (!dict.Get("statusLine", &status_line))
        status_line = request.status_line;
      v8::Local<v8::Value> value;
      if (dict.Get("responseHeaders", &value) && value->IsObject()) {
        user_modified_headers = true;
        override_headers->ReplaceStatusLine(status_line);
        gin::Converter<net::HttpResponseHeaders*>::FromV8(
            isolate, value, override_headers.get());
      }
    }
  }

  if (user_modified_headers)
    request.override_response_headers->swap(override_headers);

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(request.callback), result));
}

void WebRequest::OnSendHeaders(extensions::WebRequestInfo* info,
                               const network::ResourceRequest& request,
                               const net::HttpRequestHeaders& headers) {
  HandleSimpleEvent(SimpleEvent::kOnSendHeaders, info, request, headers);
}

WebRequest::AuthRequiredResponse WebRequest::OnAuthRequired(
    const extensions::WebRequestInfo* request_info,
    const net::AuthChallengeInfo& auth_info,
    WebRequest::AuthCallback callback,
    net::AuthCredentials* credentials) {
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(request_info->global_id);
  content::WebContents* web_contents = nullptr;
  if (rfh)
    web_contents = content::WebContents::FromRenderFrameHost(rfh);

  BlockedRequest blocked_request;
  blocked_request.auth_callback = std::move(callback);
  blocked_requests_[request_info->id] = std::move(blocked_request);

  auto login_callback =
      base::BindOnce(&WebRequest::OnLoginAuthResult, base::Unretained(this),
                     request_info->id, credentials);

  scoped_refptr<net::HttpResponseHeaders> response_headers =
      request_info->response_headers;
  blocked_requests_[request_info->id].login_handler =
      std::make_unique<LoginHandler>(
          auth_info, web_contents,
          static_cast<base::ProcessId>(
              request_info->global_id.child_id.GetUnsafeValue()),
          request_info->url, response_headers, std::move(login_callback));

  return AuthRequiredResponse::AUTH_REQUIRED_RESPONSE_IO_PENDING;
}

void WebRequest::OnBeforeRedirect(extensions::WebRequestInfo* info,
                                  const network::ResourceRequest& request,
                                  const GURL& new_location) {
  HandleSimpleEvent(SimpleEvent::kOnBeforeRedirect, info, request,
                    new_location);
}

void WebRequest::OnResponseStarted(extensions::WebRequestInfo* info,
                                   const network::ResourceRequest& request) {
  HandleSimpleEvent(SimpleEvent::kOnResponseStarted, info, request);
}

void WebRequest::OnErrorOccurred(extensions::WebRequestInfo* info,
                                 const network::ResourceRequest& request,
                                 int net_error) {
  blocked_requests_.erase(info->id);

  HandleSimpleEvent(SimpleEvent::kOnErrorOccurred, info, request, net_error);
}

void WebRequest::OnCompleted(extensions::WebRequestInfo* info,
                             const network::ResourceRequest& request,
                             int net_error) {
  blocked_requests_.erase(info->id);

  HandleSimpleEvent(SimpleEvent::kOnCompleted, info, request, net_error);
}

void WebRequest::OnRequestWillBeDestroyed(extensions::WebRequestInfo* info) {
  blocked_requests_.erase(info->id);
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

  // { urls, excludeUrls, types }.
  std::set<std::string> filter_include_patterns, filter_exclude_patterns,
      filter_types;
  RequestFilter filter;

  gin::Dictionary dict(args->isolate());
  if (args->GetNext(&arg) && !arg->IsFunction()) {
    // Note that gin treats Function as Dictionary when doing conversions, so we
    // have to explicitly check if the argument is Function before trying to
    // convert it to Dictionary.
    if (gin::ConvertFromV8(args->isolate(), arg, &dict)) {
      if (!dict.Get("urls", &filter_include_patterns)) {
        args->ThrowTypeError("Parameter 'filter' must have property 'urls'.");
        return;
      }

      if (filter_include_patterns.empty()) {
        util::EmitDeprecationWarning(
            "The urls array in WebRequestFilter is empty, which is deprecated. "
            "Please use '<all_urls>' to match all URLs.");
        filter_include_patterns.insert("<all_urls>");
      }

      dict.Get("excludeUrls", &filter_exclude_patterns);
      dict.Get("types", &filter_types);
      args->GetNext(&arg);
    }
  } else {
    // If no filter is defined, create one with <all_urls> so it matches all
    // requests
    dict = gin::Dictionary::CreateEmpty(args->isolate());
    filter_include_patterns.insert("<all_urls>");
    dict.Set("urls", filter_include_patterns);
  }

  filter.AddUrlPatterns(filter_include_patterns, &filter, args);
  filter.AddUrlPatterns(filter_exclude_patterns, &filter, args, false);

  for (const std::string& filter_type : filter_types) {
    auto type = ParseResourceType(filter_type);
    if (type != extensions::WebRequestResourceType::OTHER) {
      filter.AddType(type);
    } else {
      args->ThrowTypeError("Invalid type " + filter_type);
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
    (*listeners)[event] = {std::move(filter), std::move(listener)};
  UpdateInterceptState();
}

void WebRequest::UpdateInterceptState() {
  if (!browser_context_)
    return;
  uint32_t blocking = 0, observers = 0;
  for (const auto& [event, info] : response_listeners_)
    blocking |= info.filter.TypeMask();
  for (const auto& [event, info] : simple_listeners_)
    observers |= info.filter.TypeMask();
  browser_context_->intercept_state()->SetListenerTypes(blocking, observers);
}

struct WebRequest::ObservedRequest {
  ObservedRequest(uint64_t id,
                  content::GlobalRenderFrameHostId frame,
                  const network::ResourceRequest& request)
      : id(id), frame(frame), request(request) {
    RebuildInfo();
  }

  // WebRequestInfo is immutable; like ProxyingURLLoaderFactory, build a fresh
  // one (same id) when a redirect changes the request.
  void RebuildInfo() {
    info = std::make_unique<extensions::WebRequestInfo>(
        extensions::WebRequestInfoInitParams(
            id, frame, nullptr, request, /*is_download=*/false,
            /*is_async=*/true, /*is_service_worker_script=*/false,
            /*navigation_id=*/std::nullopt));
  }

  const uint64_t id;
  const content::GlobalRenderFrameHostId frame;
  network::ResourceRequest request;
  std::unique_ptr<extensions::WebRequestInfo> info;
};

// Stands in for the URL loader requests that net.fetch() of a file: URL no
// longer makes. Like ProxyingURLLoaderFactory, it builds a WebRequestInfo and
// calls the On* entry points, so listeners get the same details.
class WebRequest::FileFetches final : public NativePeer<WebRequest> {
 public:
  explicit FileFetches(WebRequest* web_request)
      : NativePeer<WebRequest>(web_request) {
    StartObservingShutdown();
  }

  // |observed| is a request only observers can see: the URL loader's would
  // take the observed route, which tells them about the request headers.
  [[nodiscard]] uint64_t Start(const network::ResourceRequest& request,
                               const bool observed) {
    const uint64_t id = ElectronBrowserClient::Get()->NextWebRequestId();
    auto& fetch = fetches_[id];
    // No frame: what ProxyingURLLoaderFactory gets for a browser process
    // request such as net.fetch().
    fetch.request = std::make_unique<ObservedRequest>(
        id, content::GlobalRenderFrameHostId(), request);
    if (auto web_request = wrapper(); web_request && observed) {
      ObservedRequest* const observed_request = fetch.request.get();
      Dispatch([&] {
        web_request->OnSendHeaders(observed_request->info.get(),
                                   observed_request->request,
                                   observed_request->request.headers);
      });
    }
    return id;
  }

  void BeforeRequest(const uint64_t id,
                     gin_helper::Promise<v8::Local<v8::Value>> promise) {
    auto web_request = wrapper();
    const auto it = fetches_.find(id);
    if (!web_request || !is_active() || it == fetches_.end()) {
      promise.Resolve();
      return;
    }
    Fetch& fetch = it->second;
    fetch.verdict = std::move(promise);
    int result = net::ERR_IO_PENDING;
    Dispatch([&] {
      result = web_request->OnBeforeRequest(
          fetch.request->info.get(), fetch.request->request,
          base::BindOnce(&FileFetches::OnBeforeRequestResult,
                         weak_factory_.GetWeakPtr(), id),
          &fetch.redirect_url);
    });
    if (result != net::ERR_IO_PENDING)
      OnBeforeRequestResult(id, result);
  }

  void Respond(const uint64_t id,
               const std::string& mime_type,
               const std::string& last_modified) {
    auto node = fetches_.extract(id);
    auto web_request = wrapper();
    if (!node || !web_request)
      return;
    ObservedRequest& request = *node.mapped().request;
    // What FileURLLoader sends net.fetch(): AsarURLLoaderFactory's empty
    // headers, plus the Content-Type and Last-Modified the loader adds.
    auto head = network::mojom::URLResponseHead::New();
    head->headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
    head->headers->AddHeader(net::HttpRequestHeaders::kContentType, mime_type);
    head->headers->AddHeader(net::HttpResponseHeaders::kLastModified,
                             last_modified);
    head->mime_type = mime_type;
    request.info->AddResponseInfoFromResourceResponse(*head);
    Dispatch([&] {
      web_request->OnResponseStarted(request.info.get(), request.request);
    });
    // The loader completes a request for a file that fits in its data pipe
    // before anything reads the body; net.fetch() leaves larger files to it.
    Dispatch([&] {
      web_request->OnCompleted(request.info.get(), request.request, net::OK);
    });
    web_request->OnRequestWillBeDestroyed(request.info.get());
  }

  void Abort(const uint64_t id) { Fail(id, net::ERR_ABORTED); }

 private:
  struct Fetch {
    std::unique_ptr<ObservedRequest> request;
    // Where an onBeforeRequest listener redirected the request, if it did.
    GURL redirect_url;
    std::optional<gin_helper::Promise<v8::Local<v8::Value>>> verdict;
  };

  ~FileFetches() override = default;

  // Calls listeners as the URL loader's events do, from a task of their own:
  // an exception one throws is uncaught, and the request goes on.
  template <typename Callback>
  static void Dispatch(const Callback& dispatch) {
    v8::Isolate* const isolate = JavascriptEnvironment::GetIsolate();
    v8::TryCatch try_catch(isolate);
    dispatch();
    if (try_catch.HasCaught())
      node::errors::TriggerUncaughtException(isolate, try_catch);
  }

  // NativePeer:
  void TearDownNative() override {
    weak_factory_.InvalidateWeakPtrs();
    auto web_request = wrapper();
    for (auto& [id, fetch] : std::exchange(fetches_, {})) {
      // A blocked request points at |fetch.redirect_url|.
      if (web_request)
        web_request->OnRequestWillBeDestroyed(fetch.request->info.get());
      if (fetch.verdict)
        fetch.verdict->Resolve();
    }
  }

  void OnBeforeRequestResult(const uint64_t id, const int result) {
    const auto it = fetches_.find(id);
    if (it == fetches_.end() || !it->second.verdict)
      return;
    gin_helper::Promise<v8::Local<v8::Value>> promise =
        *std::exchange(it->second.verdict, std::nullopt);
    v8::Isolate* const isolate = promise.isolate();
    v8::HandleScope handle_scope(isolate);
    auto verdict = gin_helper::Dictionary::CreateEmpty(isolate);
    if (result != net::OK) {
      verdict.Set("error", net::ErrorToString(result));
      Fail(id, result);
    } else if (const GURL redirect_url = it->second.redirect_url;
               redirect_url.is_valid()) {
      // net.fetch() then fetches the new URL as a new request, as
      // SimpleURLLoaderWrapper::OnRedirect() restarts one, unless it is a URL
      // that a redirect from a file must not reach.
      verdict.Set("redirectURL", redirect_url);
      if (!content::IsSafeRedirectTarget(it->second.request->request.url,
                                         redirect_url)) {
        verdict.Set("error", net::ErrorToString(net::ERR_UNSAFE_REDIRECT));
      }
      Fail(id, net::ERR_ABORTED, &redirect_url);
    }
    promise.Resolve(verdict.GetHandle());
  }

  // Ends the request with onErrorOccurred, after onBeforeRedirect when a
  // listener redirected it, as ProxyingURLLoaderFactory reports a redirect.
  void Fail(const uint64_t id,
            const int net_error,
            const GURL* const redirect_url = nullptr) {
    auto node = fetches_.extract(id);
    if (!node)
      return;
    Fetch& fetch = node.mapped();
    if (auto web_request = wrapper()) {
      ObservedRequest& request = *fetch.request;
      if (redirect_url) {
        request.info->AddResponseInfoFromResourceResponse(
            *CreateWebRequestRedirectHead(*redirect_url));
        Dispatch([&] {
          web_request->OnBeforeRedirect(request.info.get(), request.request,
                                        *redirect_url);
        });
      }
      Dispatch([&] {
        web_request->OnErrorOccurred(request.info.get(), request.request,
                                     net_error);
      });
      web_request->OnRequestWillBeDestroyed(request.info.get());
    }
    if (fetch.verdict)
      fetch.verdict->Resolve();
  }

  std::map<uint64_t, Fetch> fetches_;
  base::WeakPtrFactory<FileFetches> weak_factory_{this};
};

v8::Local<v8::Value> WebRequest::StartFileFetch(gin::Arguments* const args) {
  v8::Isolate* const isolate = args->isolate();
  if (!HasListener() || !browser_context_)
    return v8::Null(isolate);
  gin_helper::Dictionary opts;
  if (!args->GetNext(&opts)) {
    args->ThrowTypeError("Expected a dictionary");
    return {};
  }
  const std::unique_ptr<network::ResourceRequest> request =
      SimpleURLLoaderWrapper::CreateRequest(args, opts);
  if (!request)
    return {};
  // As ProxyingURLLoaderFactory routes the loader's request.
  const InterceptState::Route route =
      browser_context_->intercept_state()->RouteFor(*request);
  if (route == InterceptState::Route::kDirect)
    return v8::Null(isolate);
  if (!file_fetches_)
    file_fetches_ = NativePeer<WebRequest>::Create<FileFetches>(this);
  if (!file_fetches_->is_active())
    return v8::Null(isolate);
  return gin::ConvertToV8(
      isolate,
      file_fetches_->Start(*request, route == InterceptState::Route::kObserve));
}

v8::Local<v8::Promise> WebRequest::FileFetchBeforeRequest(
    v8::Isolate* const isolate,
    const uint64_t id) {
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  if (file_fetches_)
    file_fetches_->BeforeRequest(id, std::move(promise));
  else
    promise.Resolve();
  return handle;
}

void WebRequest::FileFetchResponded(const uint64_t id,
                                    const std::string& mime_type,
                                    const std::string& last_modified) {
  if (file_fetches_)
    file_fetches_->Respond(id, mime_type, last_modified);
}

void WebRequest::AbortFileFetch(const uint64_t id) {
  if (file_fetches_)
    file_fetches_->Abort(id);
}

WebRequest::WebRequest(base::PassKey<Session>,
                       base::WeakPtr<ElectronBrowserContext> browser_context)
    : browser_context_{std::move(browser_context)} {}
WebRequest::~WebRequest() = default;

// static
void WebRequest::ObservedRequestStarted(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    uint64_t key,
    int render_process_id,
    int frame_routing_id,
    const network::ResourceRequest& request) {
  auto* self = ForObservedRequest(browser_context);
  if (!self)
    return;
  auto observed = std::make_unique<ObservedRequest>(
      ElectronBrowserClient::Get()->NextWebRequestId(),
      content::GlobalRenderFrameHostId(render_process_id, frame_routing_id),
      request);
  auto& entry = *observed;
  self->observed_requests_.emplace(key, std::move(observed));
  self->OnSendHeaders(entry.info.get(), entry.request, entry.request.headers);
}

// static
void WebRequest::ObservedRequestRedirected(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    uint64_t key,
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr head) {
  auto* self = ForObservedRequest(browser_context);
  if (!self)
    return;
  auto it = self->observed_requests_.find(key);
  if (it == self->observed_requests_.end())
    return;
  ObservedRequest& observed = *it->second;
  observed.info->AddResponseInfoFromResourceResponse(*head);
  self->OnBeforeRedirect(observed.info.get(), observed.request,
                         redirect_info.new_url);
}

// static
void WebRequest::ObservedRequestFollowedRedirect(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    uint64_t key,
    const network::ResourceRequest& request) {
  auto* self = ForObservedRequest(browser_context);
  if (!self)
    return;
  auto it = self->observed_requests_.find(key);
  if (it == self->observed_requests_.end())
    return;
  ObservedRequest& observed = *it->second;
  observed.request = request;
  observed.RebuildInfo();
  self->OnSendHeaders(observed.info.get(), observed.request,
                      observed.request.headers);
}

// static
void WebRequest::ObservedRequestResponded(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    uint64_t key,
    network::mojom::URLResponseHeadPtr head) {
  auto* self = ForObservedRequest(browser_context);
  if (!self)
    return;
  auto it = self->observed_requests_.find(key);
  if (it == self->observed_requests_.end())
    return;
  ObservedRequest& observed = *it->second;
  observed.info->AddResponseInfoFromResourceResponse(*head);
  self->OnResponseStarted(observed.info.get(), observed.request);
}

// static
void WebRequest::ObservedRequestFinished(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    uint64_t key,
    const network::URLLoaderCompletionStatus& status) {
  auto* self = ForObservedRequest(browser_context);
  if (!self)
    return;
  auto it = self->observed_requests_.find(key);
  if (it == self->observed_requests_.end())
    return;
  auto observed = std::move(it->second);
  self->observed_requests_.erase(it);
  if (status.error_code == net::OK)
    self->OnCompleted(observed->info.get(), observed->request,
                      status.error_code);
  else
    self->OnErrorOccurred(observed->info.get(), observed->request,
                          status.error_code);
  self->OnRequestWillBeDestroyed(observed->info.get());
}

template <typename... Args>
void WebRequest::HandleSimpleEvent(SimpleEvent event,
                                   extensions::WebRequestInfo* request_info,
                                   Args... args) {
  const auto iter = simple_listeners_.find(event);
  if (iter == std::end(simple_listeners_))
    return;

  const auto& info = iter->second;
  if (!info.filter.MatchesRequest(request_info))
    return;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary details(isolate, v8::Object::New(isolate));
  FillDetails(&details, request_info, args...);
  info.listener.Run(gin::ConvertToV8(isolate, details));
}

void WebRequest::OnLoginAuthResult(
    uint64_t id,
    net::AuthCredentials* credentials,
    const std::optional<net::AuthCredentials>& maybe_creds) {
  auto nh = blocked_requests_.extract(id);
  CHECK(nh);

  AuthRequiredResponse action =
      AuthRequiredResponse::AUTH_REQUIRED_RESPONSE_NO_ACTION;
  if (maybe_creds.has_value()) {
    *credentials = maybe_creds.value();
    action = AuthRequiredResponse::AUTH_REQUIRED_RESPONSE_SET_AUTH;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(nh.mapped().auth_callback), action));
}

// static
WebRequest* WebRequest::FromOrCreate(v8::Isolate* isolate,
                                     content::BrowserContext* browser_context) {
  return Session::FromOrCreate(isolate, browser_context)->WebRequest(isolate);
}

// static
WebRequest* WebRequest::Create(
    v8::Isolate* isolate,
    base::PassKey<Session> passkey,
    base::WeakPtr<ElectronBrowserContext> browser_context) {
  return cppgc::MakeGarbageCollected<WebRequest>(
      isolate->GetCppHeap()->GetAllocationHandle(), passkey,
      std::move(browser_context));
}

}  // namespace electron::api
