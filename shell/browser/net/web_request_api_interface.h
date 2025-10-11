// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_WEB_REQUEST_API_INTERFACE_H_
#define ELECTRON_SHELL_BROWSER_NET_WEB_REQUEST_API_INTERFACE_H_

#include <set>
#include <string>

#include "net/base/completion_once_callback.h"
#include "services/network/public/cpp/resource_request.h"

namespace extensions {
struct WebRequestInfo;
}  // namespace extensions

namespace electron {

// Defines the interface for WebRequest API, implemented by api::WebRequestNS.
class WebRequestAPI {
 public:
  virtual ~WebRequestAPI() = default;

  // AuthRequiredResponse indicates how an OnAuthRequired call is handled.
  enum class AuthRequiredResponse {
    // No credentials were provided.
    AUTH_REQUIRED_RESPONSE_NO_ACTION,
    // AuthCredentials is filled in with a username and password, which should
    // be used in a response to the provided auth challenge.
    AUTH_REQUIRED_RESPONSE_SET_AUTH,
    // The request should be canceled.
    AUTH_REQUIRED_RESPONSE_CANCEL_AUTH,
    // The action will be decided asynchronously. `callback` will be invoked
    // when the decision is made, and one of the other AuthRequiredResponse
    // values will be passed in with the same semantics as described above.
    AUTH_REQUIRED_RESPONSE_IO_PENDING,
  };

  using BeforeSendHeadersCallback =
      base::OnceCallback<void(const std::set<std::string>& removed_headers,
                              const std::set<std::string>& set_headers,
                              int error_code)>;
  using AuthCallback = base::OnceCallback<void(AuthRequiredResponse)>;

  virtual bool HasListener() const = 0;
  virtual int OnBeforeRequest(extensions::WebRequestInfo* info,
                              const network::ResourceRequest& request,
                              net::CompletionOnceCallback callback,
                              GURL* new_url) = 0;
  virtual int OnBeforeSendHeaders(extensions::WebRequestInfo* info,
                                  const network::ResourceRequest& request,
                                  BeforeSendHeadersCallback callback,
                                  net::HttpRequestHeaders* headers) = 0;
  virtual int OnHeadersReceived(
      extensions::WebRequestInfo* info,
      const network::ResourceRequest& request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) = 0;
  virtual void OnSendHeaders(extensions::WebRequestInfo* info,
                             const network::ResourceRequest& request,
                             const net::HttpRequestHeaders& headers) = 0;
  virtual AuthRequiredResponse OnAuthRequired(
      const extensions::WebRequestInfo* info,
      const net::AuthChallengeInfo& auth_info,
      AuthCallback callback,
      net::AuthCredentials* credentials) = 0;
  virtual void OnBeforeRedirect(extensions::WebRequestInfo* info,
                                const network::ResourceRequest& request,
                                const GURL& new_location) = 0;
  virtual void OnResponseStarted(extensions::WebRequestInfo* info,
                                 const network::ResourceRequest& request) = 0;
  virtual void OnErrorOccurred(extensions::WebRequestInfo* info,
                               const network::ResourceRequest& request,
                               int net_error) = 0;
  virtual void OnCompleted(extensions::WebRequestInfo* info,
                           const network::ResourceRequest& request,
                           int net_error) = 0;
  virtual void OnRequestWillBeDestroyed(extensions::WebRequestInfo* info) = 0;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_WEB_REQUEST_API_INTERFACE_H_
