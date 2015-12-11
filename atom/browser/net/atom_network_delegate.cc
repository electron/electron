// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_network_delegate.h"

#include "atom/common/native_mate_converters/net_converter.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;

namespace atom {

namespace {

const char* ResourceTypeToString(content::ResourceType type) {
  switch (type) {
    case content::RESOURCE_TYPE_MAIN_FRAME:
      return "mainFrame";
    case content::RESOURCE_TYPE_SUB_FRAME:
      return "subFrame";
    case content::RESOURCE_TYPE_STYLESHEET:
      return "stylesheet";
    case content::RESOURCE_TYPE_SCRIPT:
      return "script";
    case content::RESOURCE_TYPE_IMAGE:
      return "image";
    case content::RESOURCE_TYPE_OBJECT:
      return "object";
    case content::RESOURCE_TYPE_XHR:
      return "xhr";
    default:
      return "other";
  }
}

AtomNetworkDelegate::BlockingResponse RunListener(
    const AtomNetworkDelegate::Listener& callback,
    scoped_ptr<base::DictionaryValue> details) {
  return callback.Run(*(details.get()));
}

bool MatchesFilterCondition(
    net::URLRequest* request,
    const AtomNetworkDelegate::ListenerInfo& info) {
  if (!info.url_patterns.empty()) {
    auto url = request->url();
    for (auto& pattern : info.url_patterns)
      if (pattern.MatchesURL(url))
        return true;
    return false;
  }

  return true;
}

void FillDetailsObject(base::DictionaryValue* details,
                       net::URLRequest* request) {
  details->SetInteger("id", request->identifier());
  details->SetString("url", request->url().spec());
  details->SetString("method", request->method());
  details->SetDouble("timestamp", base::Time::Now().ToDoubleT() * 1000);
  auto info = content::ResourceRequestInfo::ForRequest(request);
  details->SetString("resourceType",
                     info ? ResourceTypeToString(info->GetResourceType())
                          : "other");
}

void FillDetailsObject(base::DictionaryValue* details,
                       const net::HttpRequestHeaders& headers) {
  scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  net::HttpRequestHeaders::Iterator it(headers);
  while (it.GetNext())
    dict->SetString(it.name(), it.value());
  details->Set("requestHeaders", dict.Pass());
}

void FillDetailsObject(base::DictionaryValue* details,
                       net::HttpResponseHeaders* headers) {
  if (!headers)
    return;

  scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  void* iter = nullptr;
  std::string key;
  std::string value;
  while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
    key = base::ToLowerASCII(key);
    if (dict->HasKey(key)) {
      base::ListValue* values = nullptr;
      if (dict->GetList(key, &values))
        values->AppendString(value);
    } else {
      scoped_ptr<base::ListValue> values(new base::ListValue);
      values->AppendString(value);
      dict->Set(key, values.Pass());
    }
  }
  details->Set("responseHeaders", dict.Pass());
  details->SetString("statusLine", headers->GetStatusLine());
  details->SetInteger("statusCode", headers->response_code());
}

void FillDetailsObject(base::DictionaryValue* details, const GURL& location) {
  details->SetString("redirectURL", location.spec());
}

void FillDetailsObject(base::DictionaryValue* details,
                       const net::HostPortPair& host_port) {
  if (host_port.host().empty())
    details->SetString("ip", host_port.host());
}

void FillDetailsObject(base::DictionaryValue* details, bool from_cache) {
  details->SetBoolean("fromCache", from_cache);
}

void FillDetailsObject(base::DictionaryValue* details,
                       const net::URLRequestStatus& status) {
  details->SetString("error", net::ErrorToString(status.error()));
}

void OnBeforeURLRequestResponse(
    const net::CompletionCallback& callback,
    GURL* new_url,
    const AtomNetworkDelegate::BlockingResponse& result) {
  if (!result.redirect_url.is_empty())
    *new_url = result.redirect_url;
  callback.Run(result.Code());
}

void OnBeforeSendHeadersResponse(
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers,
    const AtomNetworkDelegate::BlockingResponse& result) {
  if (!result.request_headers.IsEmpty())
    *headers = result.request_headers;
  callback.Run(result.Code());
}

void OnHeadersReceivedResponse(
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    const AtomNetworkDelegate::BlockingResponse& result) {
  if (result.response_headers.get()) {
    *override_response_headers = new net::HttpResponseHeaders(
        original_response_headers->raw_headers());
    void* iter = nullptr;
    std::string key;
    std::string value;
    while (result.response_headers->EnumerateHeaderLines(&iter, &key, &value)) {
      (*override_response_headers)->RemoveHeader(key);
      (*override_response_headers)->AddHeader(key + ": " + value);
    }
  }
  callback.Run(result.Code());
}

}  // namespace

AtomNetworkDelegate::AtomNetworkDelegate() {
}

AtomNetworkDelegate::~AtomNetworkDelegate() {
}

void AtomNetworkDelegate::SetListenerInIO(
    EventType type,
    scoped_ptr<base::DictionaryValue> filter,
    const Listener& callback) {
  if (callback.is_null()) {
    event_listener_map_.erase(type);
    return;
  }

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
  auto listener_info = event_listener_map_.find(kOnBeforeRequest);
  if (listener_info != event_listener_map_.end()) {
    if (!MatchesFilterCondition(request, listener_info->second))
      return net::OK;

    scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    FillDetailsObject(details.get(), request);

    auto wrapped_callback = listener_info->second.callback;
    BrowserThread::PostTaskAndReplyWithResult(BrowserThread::UI, FROM_HERE,
        base::Bind(&RunListener, wrapped_callback, base::Passed(&details)),
        base::Bind(&OnBeforeURLRequestResponse,
                   callback, new_url));

    return net::ERR_IO_PENDING;
  }

  return brightray::NetworkDelegate::OnBeforeURLRequest(request,
                                                        callback,
                                                        new_url);
}

int AtomNetworkDelegate::OnBeforeSendHeaders(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  auto listener_info = event_listener_map_.find(kOnBeforeSendHeaders);
  if (listener_info != event_listener_map_.end()) {
    if (!MatchesFilterCondition(request, listener_info->second))
      return net::OK;

    scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    FillDetailsObject(details.get(), request);
    FillDetailsObject(details.get(), *headers);

    auto wrapped_callback = listener_info->second.callback;
    BrowserThread::PostTaskAndReplyWithResult(BrowserThread::UI, FROM_HERE,
        base::Bind(&RunListener, wrapped_callback, base::Passed(&details)),
        base::Bind(&OnBeforeSendHeadersResponse,
                   callback, headers));

    return net::ERR_IO_PENDING;
  }

  return brightray::NetworkDelegate::OnBeforeSendHeaders(request,
                                                         callback,
                                                         headers);
}

void AtomNetworkDelegate::OnSendHeaders(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
  auto listener_info = event_listener_map_.find(kOnSendHeaders);
  if (listener_info != event_listener_map_.end()) {
    if (!MatchesFilterCondition(request, listener_info->second))
      return;

    scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    FillDetailsObject(details.get(), request);
    FillDetailsObject(details.get(), headers);

    auto wrapped_callback = listener_info->second.callback;
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(&RunListener),
                                       wrapped_callback,
                                       base::Passed(&details)));
  } else {
    brightray::NetworkDelegate::OnSendHeaders(request, headers);
  }
}

int AtomNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  auto listener_info = event_listener_map_.find(kOnHeadersReceived);
  if (listener_info != event_listener_map_.end()) {
    if (!MatchesFilterCondition(request, listener_info->second))
      return net::OK;

    scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    FillDetailsObject(details.get(), request);
    FillDetailsObject(details.get(), original_response_headers);

    auto wrapped_callback = listener_info->second.callback;
    BrowserThread::PostTaskAndReplyWithResult(BrowserThread::UI, FROM_HERE,
        base::Bind(&RunListener, wrapped_callback, base::Passed(&details)),
        base::Bind(&OnHeadersReceivedResponse,
                   callback,
                   original_response_headers,
                   override_response_headers));

    return net::ERR_IO_PENDING;
  }

  return brightray::NetworkDelegate::OnHeadersReceived(
      request, callback, original_response_headers, override_response_headers,
      allowed_unsafe_redirect_url);
}

void AtomNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                           const GURL& new_location) {
  auto listener_info = event_listener_map_.find(kOnBeforeRedirect);
  if (listener_info != event_listener_map_.end()) {
    if (!MatchesFilterCondition(request, listener_info->second))
      return;

    scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    FillDetailsObject(details.get(), request);
    FillDetailsObject(details.get(), new_location);
    FillDetailsObject(details.get(), request->response_headers());
    FillDetailsObject(details.get(), request->GetSocketAddress());
    FillDetailsObject(details.get(), request->was_cached());

    auto wrapped_callback = listener_info->second.callback;
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(&RunListener),
                                       wrapped_callback,
                                       base::Passed(&details)));
  } else {
    brightray::NetworkDelegate::OnBeforeRedirect(request, new_location);
  }
}

void AtomNetworkDelegate::OnResponseStarted(net::URLRequest* request) {
  auto listener_info = event_listener_map_.find(kOnResponseStarted);
  if (listener_info != event_listener_map_.end()) {
    if (request->status().status() != net::URLRequestStatus::SUCCESS)
      return;

    if (!MatchesFilterCondition(request, listener_info->second))
      return;

    scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    FillDetailsObject(details.get(), request);
    FillDetailsObject(details.get(), request->response_headers());
    FillDetailsObject(details.get(), request->was_cached());

    auto wrapped_callback = listener_info->second.callback;
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(&RunListener),
                                       wrapped_callback,
                                       base::Passed(&details)));
  } else {
    brightray::NetworkDelegate::OnResponseStarted(request);
  }
}

void AtomNetworkDelegate::OnCompleted(net::URLRequest* request, bool started) {
  auto listener_info = event_listener_map_.find(kOnCompleted);
  if (listener_info != event_listener_map_.end()) {
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

    if (!MatchesFilterCondition(request, listener_info->second))
      return;

    scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    FillDetailsObject(details.get(), request);
    FillDetailsObject(details.get(), request->response_headers());
    FillDetailsObject(details.get(), request->was_cached());

    auto wrapped_callback = listener_info->second.callback;
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(&RunListener),
                                       wrapped_callback,
                                       base::Passed(&details)));
  } else {
    brightray::NetworkDelegate::OnCompleted(request, started);
  }
}

void AtomNetworkDelegate::OnErrorOccurred(net::URLRequest* request) {
  auto listener_info = event_listener_map_.find(kOnErrorOccurred);
  if (listener_info != event_listener_map_.end()) {
    if (!MatchesFilterCondition(request, listener_info->second))
      return;

    scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    FillDetailsObject(details.get(), request);
    FillDetailsObject(details.get(), request->was_cached());
    FillDetailsObject(details.get(), request->status());

    auto wrapped_callback = listener_info->second.callback;
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(base::IgnoreResult(&RunListener),
                                       wrapped_callback,
                                       base::Passed(&details)));
  }
}

}  // namespace atom
