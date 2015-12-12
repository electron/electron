// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_network_delegate.h"

#include <string>

#include "atom/common/native_mate_converters/net_converter.h"
#include "base/stl_util.h"
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

void RunSimpleListener(const AtomNetworkDelegate::SimpleListener& listener,
                       scoped_ptr<base::DictionaryValue> details) {
  return listener.Run(*(details.get()));
}

void RunResponseListener(
    const AtomNetworkDelegate::ResponseListener& listener,
    scoped_ptr<base::DictionaryValue> details,
    const AtomNetworkDelegate::ResponseCallback& callback) {
  return listener.Run(*(details.get()), callback);
}

// Test whether the URL of |request| matches |patterns|.
bool MatchesFilterCondition(net::URLRequest* request,
                            const URLPatterns& patterns) {
  if (patterns.empty())
    return true;

  for (const auto& pattern : patterns) {
    if (pattern.MatchesURL(request->url()))
      return true;
  }
  return false;
}

// Overloaded by multiple types to fill the |details| object.
void ToDictionary(base::DictionaryValue* details, net::URLRequest* request) {
  details->SetInteger("id", request->identifier());
  details->SetString("url", request->url().spec());
  details->SetString("method", request->method());
  details->SetDouble("timestamp", base::Time::Now().ToDoubleT() * 1000);
  auto info = content::ResourceRequestInfo::ForRequest(request);
  details->SetString("resourceType",
                     info ? ResourceTypeToString(info->GetResourceType())
                          : "other");
}

void ToDictionary(base::DictionaryValue* details,
                  const net::HttpRequestHeaders& headers) {
  scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  net::HttpRequestHeaders::Iterator it(headers);
  while (it.GetNext())
    dict->SetString(it.name(), it.value());
  details->Set("requestHeaders", dict.Pass());
}

void ToDictionary(base::DictionaryValue* details,
                  const net::HttpResponseHeaders* headers) {
  if (!headers)
    return;

  scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  void* iter = nullptr;
  std::string key;
  std::string value;
  while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
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

void ToDictionary(base::DictionaryValue* details, const GURL& location) {
  details->SetString("redirectURL", location.spec());
}

void ToDictionary(base::DictionaryValue* details,
                  const net::HostPortPair& host_port) {
  if (host_port.host().empty())
    details->SetString("ip", host_port.host());
}

void ToDictionary(base::DictionaryValue* details, bool from_cache) {
  details->SetBoolean("fromCache", from_cache);
}

void ToDictionary(base::DictionaryValue* details,
                  const net::URLRequestStatus& status) {
  details->SetString("error", net::ErrorToString(status.error()));
}

// Helper function to fill |details| with arbitrary |args|.
template<typename Arg>
void FillDetailsObject(base::DictionaryValue* details, Arg arg) {
  ToDictionary(details, arg);
}

template<typename Arg, typename... Args>
void FillDetailsObject(base::DictionaryValue* details, Arg arg, Args... args) {
  ToDictionary(details, arg);
  FillDetailsObject(details, args...);
}

// Fill the native types with the result from the response object.
void ReadFromResponseObject(const base::DictionaryValue& response,
                            GURL* new_location) {
  std::string url;
  if (response.GetString("redirectURL", &url))
    *new_location = GURL(url);
}

void ReadFromResponseObject(const base::DictionaryValue& response,
                            net::HttpRequestHeaders* headers) {
  const base::DictionaryValue* dict;
  if (response.GetDictionary("requestHeaders", &dict)) {
    for (base::DictionaryValue::Iterator it(*dict);
         !it.IsAtEnd();
         it.Advance()) {
      std::string value;
      if (it.value().GetAsString(&value))
        headers->SetHeader(it.key(), value);
    }
  }
}

void ReadFromResponseObject(const base::DictionaryValue& response,
                            scoped_refptr<net::HttpResponseHeaders>* headers) {
  const base::DictionaryValue* dict;
  if (response.GetDictionary("responseHeaders", &dict)) {
    *headers = new net::HttpResponseHeaders("");
    for (base::DictionaryValue::Iterator it(*dict);
         !it.IsAtEnd();
         it.Advance()) {
      const base::ListValue* list;
      if (it.value().GetAsList(&list)) {
        (*headers)->RemoveHeader(it.key());
        for (size_t i = 0; i < list->GetSize(); ++i) {
          std::string value;
          if (list->GetString(i, &value))
            (*headers)->AddHeader(it.key() + " : " + value);
        }
      }
    }
  }
}

// Deal with the results of Listener.
template<typename T>
void OnListenerResultInIO(const net::CompletionCallback& callback,
                          T out,
                          scoped_ptr<base::DictionaryValue> response) {
  ReadFromResponseObject(*response.get(), out);

  bool cancel = false;
  response->GetBoolean("cancel", &cancel);
  callback.Run(cancel ? net::ERR_BLOCKED_BY_CLIENT : net::OK);
}

template<typename T>
void OnListenerResultInUI(const net::CompletionCallback& callback,
                          T out,
                          const base::DictionaryValue& response) {
  scoped_ptr<base::DictionaryValue> copy = response.CreateDeepCopy();
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(OnListenerResultInIO<T>, callback, out, base::Passed(&copy)));
}

}  // namespace

AtomNetworkDelegate::AtomNetworkDelegate() {
}

AtomNetworkDelegate::~AtomNetworkDelegate() {
}

void AtomNetworkDelegate::SetSimpleListenerInIO(
    SimpleEvent type,
    const URLPatterns& patterns,
    const SimpleListener& callback) {
  if (callback.is_null())
    simple_listeners_.erase(type);
  else
    simple_listeners_[type] = { patterns, callback };
}

void AtomNetworkDelegate::SetResponseListenerInIO(
    ResponseEvent type,
    const URLPatterns& patterns,
    const ResponseListener& callback) {
  if (callback.is_null())
    response_listeners_.erase(type);
  else
    response_listeners_[type] = { patterns, callback };
}

int AtomNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  if (!ContainsKey(response_listeners_, kOnBeforeRequest))
    return brightray::NetworkDelegate::OnBeforeURLRequest(
        request, callback, new_url);

  const auto& info = response_listeners_[kOnBeforeRequest];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return net::OK;

  scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
  FillDetailsObject(details.get(), request);

  ResponseCallback response =
      base::Bind(OnListenerResultInUI<GURL*>, callback, new_url);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(RunResponseListener, info.listener, base::Passed(&details),
                 response));
  return net::ERR_IO_PENDING;
}

int AtomNetworkDelegate::OnBeforeSendHeaders(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  if (!ContainsKey(response_listeners_, kOnBeforeSendHeaders))
    return brightray::NetworkDelegate::OnBeforeSendHeaders(
        request, callback, headers);

  const auto& info = response_listeners_[kOnBeforeSendHeaders];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return net::OK;

  scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
  FillDetailsObject(details.get(), request, *headers);

  ResponseCallback response =
      base::Bind(OnListenerResultInUI<net::HttpRequestHeaders*>,
                 callback, headers);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(RunResponseListener, info.listener, base::Passed(&details),
                 response));
  return net::ERR_IO_PENDING;
}

void AtomNetworkDelegate::OnSendHeaders(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
  if (!ContainsKey(simple_listeners_, kOnSendHeaders)) {
    brightray::NetworkDelegate::OnSendHeaders(request, headers);
    return;
  }

  const auto& info = simple_listeners_[kOnSendHeaders];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return;

  scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
  FillDetailsObject(details.get(), request, headers);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&RunSimpleListener, info.listener, base::Passed(&details)));
}

int AtomNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  if (!ContainsKey(response_listeners_, kOnHeadersReceived))
    return brightray::NetworkDelegate::OnHeadersReceived(
        request, callback, original_response_headers, override_response_headers,
        allowed_unsafe_redirect_url);

  const auto& info = response_listeners_[kOnHeadersReceived];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return net::OK;

  scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
  FillDetailsObject(details.get(), request, original_response_headers);

  ResponseCallback response =
      base::Bind(OnListenerResultInUI<scoped_refptr<net::HttpResponseHeaders>*>,
                 callback, override_response_headers);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(RunResponseListener, info.listener, base::Passed(&details),
                 response));
  return net::ERR_IO_PENDING;
}

void AtomNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                           const GURL& new_location) {
  if (!ContainsKey(simple_listeners_, kOnBeforeRedirect)) {
    brightray::NetworkDelegate::OnBeforeRedirect(request, new_location);
    return;
  }

  const auto& info = simple_listeners_[kOnBeforeRedirect];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return;

  scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
  FillDetailsObject(details.get(), request, new_location,
                    request->response_headers(), request->GetSocketAddress(),
                    request->was_cached());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&RunSimpleListener, info.listener, base::Passed(&details)));
}

void AtomNetworkDelegate::OnResponseStarted(net::URLRequest* request) {
  if (!ContainsKey(simple_listeners_, kOnResponseStarted)) {
    brightray::NetworkDelegate::OnResponseStarted(request);
    return;
  }

  if (request->status().status() != net::URLRequestStatus::SUCCESS)
    return;

  const auto& info = simple_listeners_[kOnResponseStarted];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return;

  scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
  FillDetailsObject(details.get(), request, request->response_headers(),
                    request->was_cached());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(RunSimpleListener, info.listener, base::Passed(&details)));
}

void AtomNetworkDelegate::OnCompleted(net::URLRequest* request, bool started) {
  if (request->status().status() == net::URLRequestStatus::FAILED ||
      request->status().status() == net::URLRequestStatus::CANCELED) {
    // Error event.
    if (ContainsKey(simple_listeners_, kOnErrorOccurred))
      OnErrorOccurred(request);
    else
      brightray::NetworkDelegate::OnCompleted(request, started);
    return;
  } else if (request->response_headers() &&
             net::HttpResponseHeaders::IsRedirectResponseCode(
                 request->response_headers()->response_code())) {
    // Redirect event.
    brightray::NetworkDelegate::OnCompleted(request, started);
    return;
  }

  if (!ContainsKey(simple_listeners_, kOnCompleted)) {
    brightray::NetworkDelegate::OnCompleted(request, started);
    return;
  }

  const auto& info = simple_listeners_[kOnCompleted];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return;

  scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
  FillDetailsObject(details.get(), request, request->response_headers(),
                    request->was_cached());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(RunSimpleListener, info.listener, base::Passed(&details)));
}

void AtomNetworkDelegate::OnErrorOccurred(net::URLRequest* request) {
  const auto& info = simple_listeners_[kOnErrorOccurred];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return;

  scoped_ptr<base::DictionaryValue> details(new base::DictionaryValue);
  FillDetailsObject(details.get(), request, request->was_cached(),
                    request->status());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(RunSimpleListener, info.listener, base::Passed(&details)));
}

}  // namespace atom
