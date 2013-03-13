// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef __brightray__network_delegate__
#define __brightray__network_delegate__

#include "net/base/network_delegate.h"

namespace brightray {

class NetworkDelegate : public net::NetworkDelegate {
public:
  NetworkDelegate();
  virtual ~NetworkDelegate();

private:
  virtual int OnBeforeURLRequest(net::URLRequest* request, const net::CompletionCallback& callback, GURL* new_url) OVERRIDE;
  virtual int OnBeforeSendHeaders(net::URLRequest* request, const net::CompletionCallback& callback, net::HttpRequestHeaders* headers) OVERRIDE;
  virtual void OnSendHeaders(net::URLRequest* request, const net::HttpRequestHeaders& headers) OVERRIDE;
  virtual int OnHeadersReceived(net::URLRequest* request, const net::CompletionCallback& callback, const net::HttpResponseHeaders* original_response_headers, scoped_refptr<net::HttpResponseHeaders>* override_response_headers) OVERRIDE;
  virtual void OnBeforeRedirect(net::URLRequest* request, const GURL& new_location) OVERRIDE;
  virtual void OnResponseStarted(net::URLRequest* request) OVERRIDE;
  virtual void OnRawBytesRead(const net::URLRequest& request, int bytes_read) OVERRIDE;
  virtual void OnCompleted(net::URLRequest* request, bool started) OVERRIDE;
  virtual void OnURLRequestDestroyed(net::URLRequest* request) OVERRIDE;
  virtual void OnPACScriptError(int line_number, const string16& error) OVERRIDE;
  virtual AuthRequiredResponse OnAuthRequired(net::URLRequest* request, const net::AuthChallengeInfo& auth_info, const AuthCallback& callback, net::AuthCredentials* credentials) OVERRIDE;
  virtual bool OnCanGetCookies(const net::URLRequest& request, const net::CookieList& cookie_list) OVERRIDE;
  virtual bool OnCanSetCookie(const net::URLRequest& request, const std::string& cookie_line, net::CookieOptions* options) OVERRIDE;
  virtual bool OnCanAccessFile(const net::URLRequest& request, const base::FilePath& path) const OVERRIDE;
  virtual bool OnCanThrottleRequest(const net::URLRequest& request) const OVERRIDE;
  virtual int OnBeforeSocketStreamConnect(net::SocketStream* stream, const net::CompletionCallback& callback) OVERRIDE;
  virtual void OnRequestWaitStateChange(const net::URLRequest& request, RequestWaitState state) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(NetworkDelegate);
};

}

#endif /* defined(__brightray__network_delegate__) */
