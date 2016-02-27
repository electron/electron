// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_NETWORK_DELEGATE_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_NETWORK_DELEGATE_H_

#include <map>
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/browser/extensions/atom_extension_system.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class InfoMap;

class AtomExtensionsNetworkDelegate : public atom::AtomNetworkDelegate {
 public:
  explicit AtomExtensionsNetworkDelegate(
      content::BrowserContext* browser_context);
  ~AtomExtensionsNetworkDelegate() override;

  static void SetAcceptAllCookies(bool accept);

 private:
  // NetworkDelegate implementation.
  int OnBeforeURLRequestInternal(
    net::URLRequest* request,
    GURL* new_url);
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;
  int OnBeforeSendHeadersInternal(
    net::URLRequest* request,
    net::HttpRequestHeaders* headers);
  int OnBeforeSendHeaders(net::URLRequest* request,
                          const net::CompletionCallback& callback,
                          net::HttpRequestHeaders* headers) override;
  void OnSendHeaders(net::URLRequest* request,
                     const net::HttpRequestHeaders& headers) override;
  int OnHeadersReceivedInternal(
    net::URLRequest* request,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url);
  int OnHeadersReceived(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;
  void OnResponseStarted(net::URLRequest* request) override;
  void OnCompleted(net::URLRequest* request, bool started) override;
  void OnURLRequestDestroyed(net::URLRequest* request) override;
  void OnPACScriptError(int line_number, const base::string16& error) override;
  net::NetworkDelegate::AuthRequiredResponse OnAuthRequired(
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      const AuthCallback& callback,
      net::AuthCredentials* credentials) override;
  extensions::InfoMap* info_map() {
    return AtomExtensionSystemFactory::GetInstance()->
        GetForBrowserContext(browser_context_)->info_map();
  }
  void RunCallback(base::Callback<int(void)> internal_callback,
                    const uint64_t request_id,
                    int previous_result);

  content::BrowserContext* browser_context_;
  std::map<uint64_t, net::CompletionCallback> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsNetworkDelegate);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_NETWORK_DELEGATE_H_
