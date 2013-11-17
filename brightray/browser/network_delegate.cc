// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/network_delegate.h"

#include <string>

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

int NetworkDelegate::OnBeforeSendHeaders(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  return net::OK;
}

void NetworkDelegate::OnSendHeaders(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
}

int NetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers) {
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
                                            const string16& error) {
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

int NetworkDelegate::OnBeforeSocketStreamConnect(
    net::SocketStream* socket,
    const net::CompletionCallback& callback) {
  return net::OK;
}

void NetworkDelegate::OnRequestWaitStateChange(
    const net::URLRequest& request,
    RequestWaitState waiting) {
}

}  // namespace brightray
