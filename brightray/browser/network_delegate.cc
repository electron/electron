// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/network_delegate.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"

namespace brightray {

namespace {

// Ignore the limit of 6 connections per host.
const char kIgnoreConnectionsLimit[] = "ignore-connections-limit";

}  // namespace

NetworkDelegate::NetworkDelegate() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kIgnoreConnectionsLimit)) {
    std::string value = command_line->GetSwitchValueASCII(kIgnoreConnectionsLimit);
    ignore_connections_limit_domains_ = base::SplitString(
        value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  }
}

NetworkDelegate::~NetworkDelegate() {
}

int NetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
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

void NetworkDelegate::OnNetworkBytesReceived(net::URLRequest* request,
                                             int64_t bytes_read) {
}

void NetworkDelegate::OnNetworkBytesSent(net::URLRequest* request,
                                         int64_t bytes_sent) {
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

bool NetworkDelegate::OnCanEnablePrivacyMode(
    const GURL& url,
    const GURL& first_party_for_cookies) const {
  return false;
}

bool NetworkDelegate::OnAreStrictSecureCookiesEnabled() const {
  return true;
}

bool NetworkDelegate::OnAreExperimentalCookieFeaturesEnabled() const {
  return true;
}

bool NetworkDelegate::OnCancelURLRequestWithPolicyViolatingReferrerHeader(
    const net::URLRequest& request,
    const GURL& target_url,
    const GURL& referrer_url) const {
  return false;
}

}  // namespace brightray
