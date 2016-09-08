// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_NETWORK_DELEGATE_H_
#define BRIGHTRAY_BROWSER_NETWORK_DELEGATE_H_

#include <string>
#include <vector>

#include "net/base/network_delegate.h"
#include "net/proxy/proxy_server.h"

namespace brightray {

class NetworkDelegate : public net::NetworkDelegate {
 public:
  NetworkDelegate();
  virtual ~NetworkDelegate();

 protected:
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;
  int OnBeforeStartTransaction(net::URLRequest* request,
                               const net::CompletionCallback& callback,
                               net::HttpRequestHeaders* headers) override;
  void OnBeforeSendHeaders(net::URLRequest* request,
                           const net::ProxyInfo& proxy_info,
                           const net::ProxyRetryInfoMap& proxy_retry_info,
                           net::HttpRequestHeaders* headers) override;
  void OnStartTransaction(net::URLRequest* request,
                          const net::HttpRequestHeaders& headers) override;
  int OnHeadersReceived(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;
  void OnResponseStarted(net::URLRequest* request) override;
  void OnNetworkBytesReceived(net::URLRequest* request,
                              int64_t bytes_read) override;
  void OnNetworkBytesSent(net::URLRequest* request,
                          int64_t bytes_sent) override;
  void OnCompleted(net::URLRequest* request, bool started) override;
  void OnURLRequestDestroyed(net::URLRequest* request) override;
  void OnPACScriptError(int line_number,
                        const base::string16& error) override;
  AuthRequiredResponse OnAuthRequired(
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      const AuthCallback& callback,
      net::AuthCredentials* credentials) override;
  bool OnCanGetCookies(const net::URLRequest& request,
                       const net::CookieList& cookie_list) override;
  bool OnCanSetCookie(const net::URLRequest& request,
                      const std::string& cookie_line,
                      net::CookieOptions* options) override;
  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& path) const override;
  bool OnCanEnablePrivacyMode(
      const GURL& url,
      const GURL& first_party_for_cookies) const override;
  bool OnAreStrictSecureCookiesEnabled() const override;
  bool OnAreExperimentalCookieFeaturesEnabled() const override;
  bool OnCancelURLRequestWithPolicyViolatingReferrerHeader(
      const net::URLRequest& request,
      const GURL& target_url,
      const GURL& referrer_url) const override;

 private:
  std::vector<std::string> ignore_connections_limit_domains_;

  DISALLOW_COPY_AND_ASSIGN(NetworkDelegate);
};

}  // namespace brightray

#endif
