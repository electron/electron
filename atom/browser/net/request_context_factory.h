// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_REQUEST_CONTEXT_FACTORY_H_
#define ATOM_BROWSER_NET_REQUEST_CONTEXT_FACTORY_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "brightray/browser/net/url_request_context_getter_factory.h"
#include "content/public/browser/browser_context.h"
#include "net/cookies/cookie_change_dispatcher.h"

namespace net {
class HostMappingRules;
class HttpAuthPreferences;
class HttpNetworkSession;
class ProxyConfigService;
class URLRequestContextStorage;
class URLRequestJobFactory;
}  // namespace net

namespace brightray {
class BrowserContext;
class NetLog;
class RequireCTDelegate;
}  // namespace brightray

namespace atom {

struct CookieDetails;

class AtomMainRequestContextFactory
    : public brightray::URLRequestContextGetterFactory {
 public:
  AtomMainRequestContextFactory(
      const base::FilePath& path,
      bool in_memory,
      bool use_cache,
      std::string user_agent,
      std::vector<std::string> cookieable_schemes,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors,
      base::WeakPtr<brightray::BrowserContext> browser_context);
  ~AtomMainRequestContextFactory() override;

  // URLRequestContextGetterFactory:
  net::URLRequestContext* Create() override;
  net::URLRequestJobFactory* job_factory() override { return job_factory_; }

 private:
  // Notify cookie changes on the UI thread.
  void NotifyCookieChange(const net::CanonicalCookie& cookie,
                          net::CookieChangeCause cause);
  // net::CookieChangeDispatcher::CookieChangedCallback implementation.
  // Called on the IO thread.
  void OnCookieChanged(const net::CanonicalCookie& cookie,
                       net::CookieChangeCause cause);

  base::FilePath base_path_;
  bool in_memory_;
  bool use_cache_;
  std::string user_agent_;
  std::vector<std::string> cookieable_schemes_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;
  brightray::NetLog* net_log_;
  net::URLRequestJobFactory* job_factory_;

  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;
  std::unique_ptr<brightray::RequireCTDelegate> ct_delegate_;
  std::unique_ptr<net::HostMappingRules> host_mapping_rules_;
  std::unique_ptr<net::HttpAuthPreferences> http_auth_preferences_;
  std::unique_ptr<net::HttpNetworkSession> http_network_session_;
  std::unique_ptr<net::URLRequestContextStorage> storage_;
  std::unique_ptr<net::CookieChangeSubscription> cookie_change_sub_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;

  base::WeakPtr<brightray::BrowserContext> browser_context_;
  base::WeakPtrFactory<AtomMainRequestContextFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomMainRequestContextFactory);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_REQUEST_CONTEXT_FACTORY_H_
