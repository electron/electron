// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_network_delegate.h"

#include "atom/common/native_mate_converters/net_converter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;

namespace atom {

namespace {

std::string ResourceTypeToString(content::ResourceType type) {
  switch (type) {
    case content::RESOURCE_TYPE_MAIN_FRAME:
      return "main_frame";
    case content::RESOURCE_TYPE_SUB_FRAME:
      return "sub_frame";
    case content::RESOURCE_TYPE_STYLESHEET:
      return "stylesheet";
    case content::RESOURCE_TYPE_SCRIPT:
      return "script";
    case content::RESOURCE_TYPE_IMAGE:
      return "image";
    case content::RESOURCE_TYPE_OBJECT:
      return "object";
    case content::RESOURCE_TYPE_XHR:
      return "xmlhttprequest";
    default:
      return "other";
  }
}

bool MatchesFilterCondition(
    net::URLRequest* request,
    const AtomNetworkDelegate::ListenerInfo& info) {
  if (!info.url_patterns.empty()) {
    auto url = request->url();
    for (auto& pattern : info.url_patterns)
      if (pattern.MatchesURL(url))
        return true;
  }

  return false;
}

base::DictionaryValue* ExtractRequestInfo(net::URLRequest* request) {
  base::DictionaryValue* dict = new base::DictionaryValue();
  dict->SetInteger("id", request->identifier());
  dict->SetString("url", request->url().spec());
  dict->SetString("method", request->method());
  content::ResourceType resourceType = content::RESOURCE_TYPE_LAST_TYPE;
  auto info = content::ResourceRequestInfo::ForRequest(request);
  if (info)
    resourceType = info->GetResourceType();
  dict->SetString("resourceType", ResourceTypeToString(resourceType));
  dict->SetDouble("timestamp", base::Time::Now().ToDoubleT() * 1000);

  return dict;
}

base::DictionaryValue* GetRequestHeadersDict(
    const net::HttpRequestHeaders& headers) {
  base::DictionaryValue* header_dict = new base::DictionaryValue();
  net::HttpRequestHeaders::Iterator it(headers);
  while (it.GetNext())
    header_dict->SetString(it.name(), it.value());
  return header_dict;
}

base::DictionaryValue* GetResponseHeadersDict(
    const net::HttpResponseHeaders* headers) {
  base::DictionaryValue* header_dict = new base::DictionaryValue();
  if (headers) {
    void* iter = nullptr;
    std::string key;
    std::string value;
    while (headers->EnumerateHeaderLines(&iter, &key, &value))
      header_dict->SetString(key, value);
  }
  return header_dict;
}

void OnBeforeURLRequestResponse(
    const net::CompletionCallback& callback,
    GURL* new_url,
    const AtomNetworkDelegate::BlockingResponse& result) {
  if (!result.redirectURL.is_empty())
    *new_url = result.redirectURL;
  callback.Run(result.Cancel());
}

void OnBeforeSendHeadersResponse(
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers,
    const AtomNetworkDelegate::BlockingResponse& result) {
  if (!result.requestHeaders.IsEmpty())
    *headers = result.requestHeaders;
  callback.Run(result.Cancel());
}

void OnHeadersReceivedResponse(
    const net::CompletionCallback& callback,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    const AtomNetworkDelegate::BlockingResponse& result) {
  if (result.responseHeaders.get())
    *override_response_headers = result.responseHeaders;
  callback.Run(result.Cancel());
}

}  // namespace

// static
std::map<AtomNetworkDelegate::EventTypes, AtomNetworkDelegate::ListenerInfo>
AtomNetworkDelegate::event_listener_map_;

AtomNetworkDelegate::AtomNetworkDelegate() {
}

AtomNetworkDelegate::~AtomNetworkDelegate() {
}

void AtomNetworkDelegate::SetListenerInIO(
    EventTypes type,
    const base::DictionaryValue* filter,
    const Listener& callback) {
  ListenerInfo info;
  info.callback = callback;

  const base::ListValue* url_list = nullptr;
  if (filter->GetList("urls", &url_list)) {
    for (size_t i = 0; i < url_list->GetSize(); ++i) {
      std::string url;
      extensions::URLPattern pattern;
      if (url_list->GetString(i, &url) &&
          pattern.Parse(url) == extensions::URLPattern::PARSE_SUCCESS) {
        info.url_patterns.insert(pattern);
      }
    }
  }

  event_listener_map_[type] = info;
}

int AtomNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  brightray::NetworkDelegate::OnBeforeURLRequest(request, callback, new_url);

  auto listener_info = event_listener_map_[kOnBeforeRequest];

  if (!MatchesFilterCondition(request, listener_info))
    return net::OK;

  if (!listener_info.callback.is_null()) {
    auto wrapped_callback = listener_info.callback;
    auto details = ExtractRequestInfo(request);

    BrowserThread::PostTaskAndReplyWithResult(BrowserThread::UI, FROM_HERE,
        base::Bind(wrapped_callback, details),
        base::Bind(&OnBeforeURLRequestResponse,
                   callback, new_url));

    return net::ERR_IO_PENDING;
  }

  return net::OK;
}

int AtomNetworkDelegate::OnBeforeSendHeaders(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  auto listener_info = event_listener_map_[kOnBeforeSendHeaders];

  if (!MatchesFilterCondition(request, listener_info))
    return net::OK;

  if (!listener_info.callback.is_null()) {
    auto wrapped_callback = listener_info.callback;
    auto details = ExtractRequestInfo(request);
    details->Set("requestHeaders", GetRequestHeadersDict(*headers));

    BrowserThread::PostTaskAndReplyWithResult(BrowserThread::UI, FROM_HERE,
        base::Bind(wrapped_callback, details),
        base::Bind(&OnBeforeSendHeadersResponse,
                   callback, headers));

    return net::ERR_IO_PENDING;
  }

  return net::OK;
}

void AtomNetworkDelegate::OnSendHeaders(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
  auto listener_info = event_listener_map_[kOnSendHeaders];

  if (!MatchesFilterCondition(request, listener_info))
    return;

  if (!listener_info.callback.is_null()) {
    auto wrapped_callback = listener_info.callback;
    auto details = ExtractRequestInfo(request);
    details->Set("requestHeaders", GetRequestHeadersDict(headers));

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(wrapped_callback),
                                       details));
  }
}

int AtomNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  auto listener_info = event_listener_map_[kOnHeadersReceived];

  if (!MatchesFilterCondition(request, listener_info))
    return net::OK;

  if (!listener_info.callback.is_null()) {
    auto wrapped_callback = listener_info.callback;
    auto details = ExtractRequestInfo(request);
    details->SetString("statusLine",
                       original_response_headers->GetStatusLine());
    details->SetInteger("statusCode",
                        original_response_headers->response_code());
    details->Set("responseHeaders",
                 GetResponseHeadersDict(original_response_headers));

    BrowserThread::PostTaskAndReplyWithResult(BrowserThread::UI, FROM_HERE,
        base::Bind(wrapped_callback, details),
        base::Bind(&OnHeadersReceivedResponse,
                   callback,
                   override_response_headers));

    return net::ERR_IO_PENDING;
  }

  return net::OK;
}

void AtomNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                            const GURL& new_location) {
  auto listener_info = event_listener_map_[kOnBeforeRedirect];

  if (!MatchesFilterCondition(request, listener_info))
    return;

  if (!listener_info.callback.is_null()) {
    auto wrapped_callback = listener_info.callback;
    auto details = ExtractRequestInfo(request);
    details->SetString("redirectURL", new_location.spec());
    details->SetInteger("statusCode", request->GetResponseCode());
    auto ip = request->GetSocketAddress().host();
    if (!ip.empty())
      details->SetString("ip", ip);
    details->SetBoolean("fromCache", request->was_cached());

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(wrapped_callback),
                                       details));
  }
}

void AtomNetworkDelegate::OnResponseStarted(net::URLRequest* request) {
  if (request->status().status() != net::URLRequestStatus::SUCCESS)
    return;

  auto listener_info = event_listener_map_[kOnResponseStarted];

  if (!MatchesFilterCondition(request, listener_info))
    return;

  if (!listener_info.callback.is_null()) {
    auto wrapped_callback = listener_info.callback;
    auto details = ExtractRequestInfo(request);
    details->Set("responseHeaders",
                 GetResponseHeadersDict(request->response_headers()));
    details->SetBoolean("fromCache", request->was_cached());
    details->SetInteger("statusCode",
                        request->response_headers() ?
                            request->response_headers()->response_code() : 200);

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(wrapped_callback),
                                       details));
  }
}

void AtomNetworkDelegate::OnCompleted(net::URLRequest* request, bool started) {
  if (request->status().status() == net::URLRequestStatus::FAILED ||
      request->status().status() == net::URLRequestStatus::CANCELED) {
    OnErrorOccurred(request);
    return;
  } else {
    bool is_redirect = request->response_headers() &&
        net::HttpResponseHeaders::IsRedirectResponseCode(
            request->response_headers()->response_code());
    if (is_redirect)
      return;
  }

  auto listener_info = event_listener_map_[kOnCompleted];

  if (!MatchesFilterCondition(request, listener_info))
    return;

  if (!listener_info.callback.is_null()) {
    auto wrapped_callback = listener_info.callback;
    auto details = ExtractRequestInfo(request);
    details->Set("responseHeaders",
                 GetResponseHeadersDict(request->response_headers()));
    details->SetBoolean("fromCache", request->was_cached());
    details->SetInteger("statusCode",
                        request->response_headers() ?
                            request->response_headers()->response_code() : 200);

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(wrapped_callback),
                                       details));
  }
}

void AtomNetworkDelegate::OnErrorOccurred(net::URLRequest* request) {
  auto listener_info = event_listener_map_[kOnErrorOccurred];

  if (!MatchesFilterCondition(request, listener_info))
    return;

  if (!listener_info.callback.is_null()) {
    auto wrapped_callback = listener_info.callback;
    auto details = ExtractRequestInfo(request);
    details->SetBoolean("fromCache", request->was_cached());
    details->SetString("error", net::ErrorToString(request->status().error()));

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(wrapped_callback),
                                       details));
  }
}

}  // namespace atom
