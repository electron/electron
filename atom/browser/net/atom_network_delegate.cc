// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_network_delegate.h"

#include <memory>
#include <utility>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/login_handler.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;

namespace atom {

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

int32_t GetWebContentsID(int process_id, int frame_id) {
  auto* webContents = content::WebContents::FromRenderFrameHost(
      content::RenderFrameHost::FromID(process_id, frame_id));
  return atom::api::WebContents::GetIDFromWrappedClass(webContents);
}

namespace {

using ResponseHeadersContainer =
    std::pair<scoped_refptr<net::HttpResponseHeaders>*, const std::string&>;

void RunSimpleListener(const AtomNetworkDelegate::SimpleListener& listener,
                       std::unique_ptr<base::DictionaryValue> details,
                       int render_process_id,
                       int render_frame_id) {
  int32_t id = GetWebContentsID(render_process_id, render_frame_id);
  // id must be greater than zero
  if (id)
    details->SetInteger("webContentsId", id);
  return listener.Run(*(details.get()));
}

void RunResponseListener(
    const AtomNetworkDelegate::ResponseListener& listener,
    std::unique_ptr<base::DictionaryValue> details,
    int render_process_id,
    int render_frame_id,
    const AtomNetworkDelegate::ResponseCallback& callback) {
  int32_t id = GetWebContentsID(render_process_id, render_frame_id);
  // id must be greater than zero
  if (id)
    details->SetInteger("webContentsId", id);
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
  FillRequestDetails(details, request);
  details->SetInteger("id", request->identifier());
  details->SetDouble("timestamp", base::Time::Now().ToDoubleT() * 1000);
  const auto* info = content::ResourceRequestInfo::ForRequest(request);
  if (info) {
    details->SetString("resourceType",
                       ResourceTypeToString(info->GetResourceType()));
  } else {
    details->SetString("resourceType", "other");
  }
}

void ToDictionary(base::DictionaryValue* details,
                  const net::HttpRequestHeaders& headers) {
  auto dict = std::make_unique<base::DictionaryValue>();
  net::HttpRequestHeaders::Iterator it(headers);
  while (it.GetNext())
    dict->SetKey(it.name(), base::Value(it.value()));
  details->Set("requestHeaders", std::move(dict));
}

void ToDictionary(base::DictionaryValue* details,
                  const net::HttpResponseHeaders* headers) {
  if (!headers)
    return;

  auto dict = std::make_unique<base::DictionaryValue>();
  size_t iter = 0;
  std::string key;
  std::string value;
  while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
    if (dict->FindKey(key)) {
      base::ListValue* values = nullptr;
      if (dict->GetList(key, &values))
        values->AppendString(value);
    } else {
      auto values = std::make_unique<base::ListValue>();
      values->AppendString(value);
      dict->Set(key, std::move(values));
    }
  }
  details->Set("responseHeaders", std::move(dict));
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
template <typename Arg>
void FillDetailsObject(base::DictionaryValue* details, Arg arg) {
  ToDictionary(details, arg);
}

template <typename Arg, typename... Args>
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
    headers->Clear();
    for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd();
         it.Advance()) {
      if (it.value().is_string()) {
        std::string value = it.value().GetString();
        headers->SetHeader(it.key(), value);
      }
    }
  }
}

void ReadFromResponseObject(const base::DictionaryValue& response,
                            const ResponseHeadersContainer& container) {
  const base::DictionaryValue* dict;
  std::string status_line;
  if (!response.GetString("statusLine", &status_line))
    status_line = container.second;
  if (response.GetDictionary("responseHeaders", &dict)) {
    auto* headers = container.first;
    *headers = new net::HttpResponseHeaders("");
    (*headers)->ReplaceStatusLine(status_line);
    for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd();
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

}  // namespace

AtomNetworkDelegate::SimpleListenerInfo::SimpleListenerInfo(
    URLPatterns patterns_,
    SimpleListener listener_)
    : url_patterns(patterns_), listener(listener_) {}
AtomNetworkDelegate::SimpleListenerInfo::SimpleListenerInfo() = default;
AtomNetworkDelegate::SimpleListenerInfo::~SimpleListenerInfo() = default;

AtomNetworkDelegate::ResponseListenerInfo::ResponseListenerInfo(
    URLPatterns patterns_,
    ResponseListener listener_)
    : url_patterns(patterns_), listener(listener_) {}
AtomNetworkDelegate::ResponseListenerInfo::ResponseListenerInfo() = default;
AtomNetworkDelegate::ResponseListenerInfo::~ResponseListenerInfo() = default;

AtomNetworkDelegate::AtomNetworkDelegate() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kIgnoreConnectionsLimit)) {
    std::string value =
        command_line->GetSwitchValueASCII(switches::kIgnoreConnectionsLimit);
    ignore_connections_limit_domains_ = base::SplitString(
        value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  }
}

AtomNetworkDelegate::~AtomNetworkDelegate() {}

void AtomNetworkDelegate::SetSimpleListenerInIO(SimpleEvent type,
                                                URLPatterns patterns,
                                                SimpleListener callback) {
  if (callback.is_null())
    simple_listeners_.erase(type);
  else
    simple_listeners_[type] = {std::move(patterns), std::move(callback)};
}

void AtomNetworkDelegate::SetResponseListenerInIO(ResponseEvent type,
                                                  URLPatterns patterns,
                                                  ResponseListener callback) {
  if (callback.is_null())
    response_listeners_.erase(type);
  else
    response_listeners_[type] = {std::move(patterns), std::move(callback)};
}

void AtomNetworkDelegate::SetDevToolsNetworkEmulationClientId(
    const base::UnguessableToken& client_id) {
  client_id_ = client_id;
}

int AtomNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  if (!base::ContainsKey(response_listeners_, kOnBeforeRequest)) {
    for (const auto& domain : ignore_connections_limit_domains_) {
      if (request->url().DomainIs(domain)) {
        // Allow unlimited concurrent connections.
        request->SetPriority(net::MAXIMUM_PRIORITY);
        request->SetLoadFlags(request->load_flags() | net::LOAD_IGNORE_LIMITS);
        break;
      }
    }
    return net::OK;
  }

  return HandleResponseEvent(kOnBeforeRequest, request, callback, new_url);
}

int AtomNetworkDelegate::OnBeforeStartTransaction(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  if (!base::ContainsKey(response_listeners_, kOnBeforeSendHeaders))
    return net::OK;

  return HandleResponseEvent(kOnBeforeSendHeaders, request, callback, headers,
                             *headers);
}

void AtomNetworkDelegate::OnStartTransaction(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
  if (!base::ContainsKey(simple_listeners_, kOnSendHeaders))
    return;

  HandleSimpleEvent(kOnSendHeaders, request, headers);
}

int AtomNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original,
    scoped_refptr<net::HttpResponseHeaders>* override,
    GURL* allowed) {
  if (!base::ContainsKey(response_listeners_, kOnHeadersReceived))
    return net::OK;

  return HandleResponseEvent(
      kOnHeadersReceived, request, callback,
      std::make_pair(override, original->GetStatusLine()), original);
}

void AtomNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                           const GURL& new_location) {
  if (!base::ContainsKey(simple_listeners_, kOnBeforeRedirect))
    return;

  HandleSimpleEvent(kOnBeforeRedirect, request, new_location,
                    request->response_headers(), request->GetSocketAddress(),
                    request->was_cached());
}

void AtomNetworkDelegate::OnResponseStarted(net::URLRequest* request,
                                            int net_error) {
  if (!base::ContainsKey(simple_listeners_, kOnResponseStarted))
    return;

  if (request->status().status() != net::URLRequestStatus::SUCCESS)
    return;

  HandleSimpleEvent(kOnResponseStarted, request, request->response_headers(),
                    request->was_cached());
}

void AtomNetworkDelegate::OnCompleted(net::URLRequest* request, bool started) {
  // OnCompleted may happen before other events.
  callbacks_.erase(request->identifier());

  if (request->status().status() == net::URLRequestStatus::FAILED ||
      request->status().status() == net::URLRequestStatus::CANCELED) {
    // Error event.
    OnErrorOccurred(request, started);
    return;
  }

  if (request->response_headers() &&
      net::HttpResponseHeaders::IsRedirectResponseCode(
          request->response_headers()->response_code())) {
    // Redirect event.
    return;
  }

  if (!base::ContainsKey(simple_listeners_, kOnCompleted))
    return;

  HandleSimpleEvent(kOnCompleted, request, request->response_headers(),
                    request->was_cached());
}

void AtomNetworkDelegate::OnURLRequestDestroyed(net::URLRequest* request) {
  const auto& it = login_handler_map_.find(request->identifier());
  if (it != login_handler_map_.end()) {
    it->second->NotifyRequestDestroyed();
    it->second = nullptr;
    login_handler_map_.erase(it);
  }
  callbacks_.erase(request->identifier());
}

net::NetworkDelegate::AuthRequiredResponse AtomNetworkDelegate::OnAuthRequired(
    net::URLRequest* request,
    const net::AuthChallengeInfo& auth_info,
    const AuthCallback& callback,
    net::AuthCredentials* credentials) {
  auto* resource_request_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!resource_request_info)
    return AUTH_REQUIRED_RESPONSE_NO_ACTION;
  login_handler_map_.emplace(
      request->identifier(),
      new LoginHandler(request, auth_info, std::move(callback), credentials,
                       resource_request_info));
  return AUTH_REQUIRED_RESPONSE_IO_PENDING;
}

bool AtomNetworkDelegate::OnCanGetCookies(const net::URLRequest& request,
                                          const net::CookieList& cookie_list) {
  return true;
}

bool AtomNetworkDelegate::OnCanSetCookie(
    const net::URLRequest& request,
    const net::CanonicalCookie& cookie_line,
    net::CookieOptions* options) {
  return true;
}

bool AtomNetworkDelegate::OnCanAccessFile(
    const net::URLRequest& request,
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
  return true;
}

bool AtomNetworkDelegate::OnCanEnablePrivacyMode(
    const GURL& url,
    const GURL& first_party_for_cookies) const {
  return false;
}

bool AtomNetworkDelegate::OnAreExperimentalCookieFeaturesEnabled() const {
  return true;
}

bool AtomNetworkDelegate::OnCancelURLRequestWithPolicyViolatingReferrerHeader(
    const net::URLRequest& request,
    const GURL& target_url,
    const GURL& referrer_url) const {
  return false;
}

// TODO(deepak1556) : Enable after hooking into the reporting service
// https://crbug.com/704259
bool AtomNetworkDelegate::OnCanQueueReportingReport(
    const url::Origin& origin) const {
  return false;
}

void AtomNetworkDelegate::OnCanSendReportingReports(
    std::set<url::Origin> origins,
    base::OnceCallback<void(std::set<url::Origin>)> result_callback) const {}

bool AtomNetworkDelegate::OnCanSetReportingClient(const url::Origin& origin,
                                                  const GURL& endpoint) const {
  return false;
}

bool AtomNetworkDelegate::OnCanUseReportingClient(const url::Origin& origin,
                                                  const GURL& endpoint) const {
  return false;
}

void AtomNetworkDelegate::OnErrorOccurred(net::URLRequest* request,
                                          bool started) {
  if (!base::ContainsKey(simple_listeners_, kOnErrorOccurred))
    return;

  HandleSimpleEvent(kOnErrorOccurred, request, request->was_cached(),
                    request->status());
}

template <typename Out, typename... Args>
int AtomNetworkDelegate::HandleResponseEvent(
    ResponseEvent type,
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    Out out,
    Args... args) {
  const auto& info = response_listeners_[type];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return net::OK;

  auto details = std::make_unique<base::DictionaryValue>();
  FillDetailsObject(details.get(), request, args...);

  int render_process_id, render_frame_id;
  content::ResourceRequestInfo::GetRenderFrameForRequest(
      request, &render_process_id, &render_frame_id);

  // The |request| could be destroyed before the |callback| is called.
  callbacks_[request->identifier()] = callback;

  ResponseCallback response =
      base::Bind(&AtomNetworkDelegate::OnListenerResultInUI<Out>,
                 base::Unretained(this), request->identifier(), out);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(RunResponseListener, info.listener, std::move(details),
                     render_process_id, render_frame_id, response));
  return net::ERR_IO_PENDING;
}

template <typename... Args>
void AtomNetworkDelegate::HandleSimpleEvent(SimpleEvent type,
                                            net::URLRequest* request,
                                            Args... args) {
  const auto& info = simple_listeners_[type];
  if (!MatchesFilterCondition(request, info.url_patterns))
    return;

  auto details = std::make_unique<base::DictionaryValue>();
  FillDetailsObject(details.get(), request, args...);

  int render_process_id, render_frame_id;
  content::ResourceRequestInfo::GetRenderFrameForRequest(
      request, &render_process_id, &render_frame_id);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(RunSimpleListener, info.listener, std::move(details),
                     render_process_id, render_frame_id));
}

template <typename T>
void AtomNetworkDelegate::OnListenerResultInIO(
    uint64_t id,
    T out,
    std::unique_ptr<base::DictionaryValue> response) {
  // The request has been destroyed.
  if (!base::ContainsKey(callbacks_, id))
    return;

  ReadFromResponseObject(*response, out);

  bool cancel = false;
  response->GetBoolean("cancel", &cancel);
  callbacks_[id].Run(cancel ? net::ERR_BLOCKED_BY_CLIENT : net::OK);
}

template <typename T>
void AtomNetworkDelegate::OnListenerResultInUI(
    uint64_t id,
    T out,
    const base::DictionaryValue& response) {
  auto copy = base::DictionaryValue::From(
      base::Value::ToUniquePtrValue(response.Clone()));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AtomNetworkDelegate::OnListenerResultInIO<T>,
                     base::Unretained(this), id, out, std::move(copy)));
}

}  // namespace atom
