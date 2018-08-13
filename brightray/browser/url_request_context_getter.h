// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_
#define BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/http/http_cache.h"
#include "net/http/transport_security_state.h"
#include "net/http/url_security_manager.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

#if DCHECK_IS_ON()
#include "base/debug/leak_tracker.h"
#endif

namespace net {
class HostMappingRules;
class HostResolver;
class HttpAuthPreferences;
class NetworkDelegate;
class ProxyConfigService;
class URLRequestContextStorage;
class URLRequestJobFactory;
}  // namespace net

namespace brightray {

class BrowserContext;
class ResourceContext;
class RequireCTDelegate;
class NetLog;

class URLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  class Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}

    virtual std::unique_ptr<net::NetworkDelegate> CreateNetworkDelegate() = 0;
    virtual std::unique_ptr<net::URLRequestJobFactory>
    CreateURLRequestJobFactory(
        net::URLRequestContext* url_request_context,
        content::ProtocolHandlerMap* protocol_handlers) = 0;
    virtual net::HttpCache::BackendFactory* CreateHttpCacheBackendFactory(
        const base::FilePath& base_path) = 0;
    virtual std::unique_ptr<net::CertVerifier> CreateCertVerifier(
        RequireCTDelegate* ct_delegate) = 0;
    virtual void GetCookieableSchemes(
        std::vector<std::string>* cookie_schemes) {}
    virtual void OnCookieChanged(const net::CanonicalCookie& cookie,
                                 net::CookieChangeCause cause) {}
  };

  URLRequestContextGetter(
      NetLog* net_log,
      ResourceContext* resource_context,
      bool in_memory,
      const std::string& user_agent,
      const base::FilePath& base_path,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector protocol_interceptors);

  // net::URLRequestContextGetter:
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

  net::URLRequestJobFactory* job_factory() const { return job_factory_; }
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Discard reference to URLRequestContext and inform observers to
  // shutdown. Must be called only on IO thread.
  void NotifyContextShuttingDown(std::unique_ptr<ResourceContext>);

 private:
  friend class BrowserContext;

  // Responsible for destroying URLRequestContextGetter
  // on the IO thread.
  class Handle {
   public:
    explicit Handle(base::WeakPtr<BrowserContext> browser_context);
    ~Handle();

    scoped_refptr<URLRequestContextGetter> CreateMainRequestContextGetter(
        content::ProtocolHandlerMap* protocol_handlers,
        content::URLRequestInterceptorScopedVector protocol_interceptors);
    content::ResourceContext* GetResourceContext() const;
    scoped_refptr<URLRequestContextGetter> GetMainRequestContextGetter() const;

    void ShutdownOnUIThread();

   private:
    void LazyInitialize() const;

    scoped_refptr<URLRequestContextGetter> main_request_context_getter_;
    std::unique_ptr<ResourceContext> resource_context_;
    base::WeakPtr<BrowserContext> browser_context_;
    mutable bool initialized_;

    DISALLOW_COPY_AND_ASSIGN(Handle);
  };

  ~URLRequestContextGetter() override;

  // net::CookieChangeDispatcher::CookieChangedCallback implementation.
  void OnCookieChanged(const net::CanonicalCookie& cookie,
                       net::CookieChangeCause cause) const;

#if DCHECK_IS_ON()
  base::debug::LeakTracker<URLRequestContextGetter> leak_tracker_;
#endif

  std::unique_ptr<RequireCTDelegate> ct_delegate_;
  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;
  std::unique_ptr<net::URLRequestContextStorage> storage_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;
  std::unique_ptr<net::HostMappingRules> host_mapping_rules_;
  std::unique_ptr<net::HttpAuthPreferences> http_auth_preferences_;
  std::unique_ptr<net::HttpNetworkSession> http_network_session_;
  std::unique_ptr<net::CookieChangeSubscription> cookie_change_sub_;

  net::URLRequestJobFactory* job_factory_;
  Delegate* delegate_;
  NetLog* net_log_;
  ResourceContext* resource_context_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector protocol_interceptors_;
  base::FilePath base_path_;
  bool in_memory_;
  std::string user_agent_;
  bool context_shutting_down_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestContextGetter);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_
