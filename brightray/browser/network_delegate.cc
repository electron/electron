// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/network_delegate.h"

#include "net/base/net_errors.h"

namespace brightray {

NetworkDelegate::NetworkDelegate() {
}

NetworkDelegate::~NetworkDelegate() {
}

int NetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  return net::OK;
}

void NetworkDelegate::OnResolveProxy(const GURL& url,
                                     int load_flags,
                                     const net::ProxyService& proxy_service,
                                     net::ProxyInfo* result) {
}

void NetworkDelegate::OnProxyFallback(const net::ProxyServer& bad_proxy, int net_error) {
}

int NetworkDelegate::OnBeforeSendHeaders(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  return net::OK;
}

void NetworkDelegate::OnBeforeSendProxyHeaders(net::URLRequest* request,
                                               const net::ProxyInfo& proxy_info,
                                               net::HttpRequestHeaders* headers) {
}

void NetworkDelegate::OnSendHeaders(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
}

int NetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return net::OK;
}

void NetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                            const GURL& new_location) {
}

void NetworkDelegate::OnResponseStarted(net::URLRequest* request) {
}

void NetworkDelegate::OnRawBytesRead(const net::URLRequest& request,
                                          int bytes_read) {
}

void NetworkDelegate::OnCompleted(net::URLRequest* request, bool started) {
}

void NetworkDelegate::OnURLRequestDestroyed(net::URLRequest* request) {
}

void NetworkDelegate::OnPACScriptError(int line_number,
                                       const base::string16& error) {
}

NetworkDelegate::AuthRequiredResponse NetworkDelegate::OnAuthRequired(
    net::URLRequest* request,
    const net::AuthChallengeInfo& auth_info,
    const AuthCallback& callback,
    net::AuthCredentials* credentials) {
  return AUTH_REQUIRED_RESPONSE_NO_ACTION;
}

bool NetworkDelegate::OnCanGetCookies(const net::URLRequest& request,
                                           const net::CookieList& cookie_list) {
  return true;
}

bool NetworkDelegate::OnCanSetCookie(const net::URLRequest& request,
                                          const std::string& cookie_line,
                                          net::CookieOptions* options) {
  return true;
}

bool NetworkDelegate::OnCanAccessFile(const net::URLRequest& request,
                                           const base::FilePath& path) const {
  return true;
}

bool NetworkDelegate::OnCanThrottleRequest(
    const net::URLRequest& request) const {
  return false;
}

bool NetworkDelegate::OnCanEnablePrivacyMode(
    const GURL& url,
    const GURL& first_party_for_cookies) const {
  return false;
}

bool NetworkDelegate::OnCancelURLRequestWithPolicyViolatingReferrerHeader(
    const net::URLRequest& request,
    const GURL& target_url,
    const GURL& referrer_url) const {
  return false;
}

}  // namespace brightray
