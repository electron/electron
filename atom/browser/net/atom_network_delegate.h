// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_
#define ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/synchronization/lock.h"
#include "base/values.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/common/url_pattern.h"
#include "net/base/network_delegate.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

class URLPattern;

namespace atom {

using URLPatterns = std::set<URLPattern>;

const char* ResourceTypeToString(content::ResourceType type);

class LoginHandler;

class AtomNetworkDelegate : public net::NetworkDelegate {
 public:
  using ResponseCallback = base::Callback<void(const base::DictionaryValue&)>;
  using SimpleListener = base::Callback<void(const base::DictionaryValue&)>;
  using ResponseListener = base::Callback<void(const base::DictionaryValue&,
                                               const ResponseCallback&)>;

  enum SimpleEvent {
    kOnSendHeaders,
    kOnBeforeRedirect,
    kOnResponseStarted,
    kOnCompleted,
    kOnErrorOccurred,
  };

  enum ResponseEvent {
    kOnBeforeRequest,
    kOnBeforeSendHeaders,
    kOnHeadersReceived,
  };

  struct SimpleListenerInfo {
    URLPatterns url_patterns;
    SimpleListener listener;

    SimpleListenerInfo(URLPatterns, SimpleListener);
    SimpleListenerInfo();
    ~SimpleListenerInfo();
  };

  struct ResponseListenerInfo {
    URLPatterns url_patterns;
    ResponseListener listener;

    ResponseListenerInfo(URLPatterns, ResponseListener);
    ResponseListenerInfo();
    ~ResponseListenerInfo();
  };

  AtomNetworkDelegate();
  ~AtomNetworkDelegate() override;

  void SetSimpleListenerInIO(SimpleEvent type,
                             URLPatterns patterns,
                             SimpleListener callback);
  void SetResponseListenerInIO(ResponseEvent type,
                               URLPatterns patterns,
                               ResponseListener callback);

 protected:
  // net::NetworkDelegate:
  int OnBeforeURLRequest(net::URLRequest* request,
                         net::CompletionOnceCallback callback,
                         GURL* new_url) override;
  int OnBeforeStartTransaction(net::URLRequest* request,
                               net::CompletionOnceCallback callback,
                               net::HttpRequestHeaders* headers) override;
  void OnBeforeSendHeaders(net::URLRequest* request,
                           const net::ProxyInfo& proxy_info,
                           const net::ProxyRetryInfoMap& proxy_retry_info,
                           net::HttpRequestHeaders* headers) override {}
  void OnStartTransaction(net::URLRequest* request,
                          const net::HttpRequestHeaders& headers) override;
  int OnHeadersReceived(
      net::URLRequest* request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnNetworkBytesReceived(net::URLRequest* request,
                              int64_t bytes_read) override {}
  void OnNetworkBytesSent(net::URLRequest* request,
                          int64_t bytes_sent) override {}
  void OnCompleted(net::URLRequest* request,
                   bool started,
                   int net_error) override;
  void OnURLRequestDestroyed(net::URLRequest* request) override;
  void OnPACScriptError(int line_number, const base::string16& error) override {
  }
  AuthRequiredResponse OnAuthRequired(
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      AuthCallback callback,
      net::AuthCredentials* credentials) override;
  bool OnCanGetCookies(const net::URLRequest& request,
                       const net::CookieList& cookie_list) override;
  bool OnCanSetCookie(const net::URLRequest& request,
                      const net::CanonicalCookie& cookie_line,
                      net::CookieOptions* options) override;
  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& original_path,
                       const base::FilePath& absolute_path) const override;
  bool OnCanEnablePrivacyMode(
      const GURL& url,
      const GURL& first_party_for_cookies) const override;
  bool OnAreExperimentalCookieFeaturesEnabled() const override;
  bool OnCancelURLRequestWithPolicyViolatingReferrerHeader(
      const net::URLRequest& request,
      const GURL& target_url,
      const GURL& referrer_url) const override;
  bool OnCanQueueReportingReport(const url::Origin& origin) const override;
  void OnCanSendReportingReports(std::set<url::Origin> origins,
                                 base::OnceCallback<void(std::set<url::Origin>)>
                                     result_callback) const override;
  bool OnCanSetReportingClient(const url::Origin& origin,
                               const GURL& endpoint) const override;
  bool OnCanUseReportingClient(const url::Origin& origin,
                               const GURL& endpoint) const override;

 private:
  void OnErrorOccurred(net::URLRequest* request, bool started, int net_error);

  template <typename... Args>
  void HandleSimpleEvent(SimpleEvent type,
                         net::URLRequest* request,
                         Args... args);
  template <typename Out, typename... Args>
  int HandleResponseEvent(ResponseEvent type,
                          net::URLRequest* request,
                          net::CompletionOnceCallback callback,
                          Out out,
                          Args... args);

  // Deal with the results of Listener.
  template <typename T>
  void OnListenerResultInIO(uint64_t id,
                            T out,
                            std::unique_ptr<base::DictionaryValue> response);
  template <typename T>
  void OnListenerResultInUI(uint64_t id,
                            T out,
                            const base::DictionaryValue& response);

  std::map<uint64_t, scoped_refptr<LoginHandler>> login_handler_map_;
  std::map<SimpleEvent, SimpleListenerInfo> simple_listeners_;
  std::map<ResponseEvent, ResponseListenerInfo> response_listeners_;
  std::map<uint64_t, net::CompletionOnceCallback> callbacks_;
  std::vector<std::string> ignore_connections_limit_domains_;

  DISALLOW_COPY_AND_ASSIGN(AtomNetworkDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_
