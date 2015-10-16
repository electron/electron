// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_
#define BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_

#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/content_browser_client.h"
#include "net/http/http_cache.h"
#include "net/http/url_security_manager.h"
#include "net/url_request/url_request_context_getter.h"

namespace base {
class MessageLoop;
}

namespace net {
class HostMappingRules;
class HostResolver;
class NetworkDelegate;
class ProxyConfigService;
class URLRequestContextStorage;
class URLRequestJobFactory;
}

namespace brightray {

class DevToolsNetworkController;
class NetLog;

class ExplicitURLSecurityManager : public net::URLSecurityManager {
 public:
  ExplicitURLSecurityManager();

  bool CanUseDefaultCredentials(const GURL& auth_origin) const override;
  bool CanDelegate(const GURL& auth_origin) const override;

  void AllowNTLMCredentialsForAllDomains(bool should_allow) { allow_default_creds_ = should_allow; }

 private:
  bool allow_default_creds_;
  scoped_ptr<net::URLSecurityManager> orig_url_sec_mgr_;

  DISALLOW_COPY_AND_ASSIGN(ExplicitURLSecurityManager);
};

class URLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  class Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}

    virtual net::NetworkDelegate* CreateNetworkDelegate() { return NULL; }
    virtual std::string GetUserAgent();
    virtual net::URLRequestJobFactory* CreateURLRequestJobFactory(
        content::ProtocolHandlerMap* protocol_handlers,
        content::URLRequestInterceptorScopedVector* protocol_interceptors);
    virtual net::HttpCache::BackendFactory* CreateHttpCacheBackendFactory(
        const base::FilePath& base_path);
    virtual net::SSLConfigService* CreateSSLConfigService();
  };

  URLRequestContextGetter(
      Delegate* delegate,
      DevToolsNetworkController* controller,
      NetLog* net_log,
      const base::FilePath& base_path,
      bool in_memory,
      base::MessageLoop* io_loop,
      base::MessageLoop* file_loop,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector protocol_interceptors);
  virtual ~URLRequestContextGetter();

  // net::URLRequestContextGetter:
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner() const override;

  void AllowNTLMCredentialsForAllDomains(bool should_allow);

  net::HostResolver* host_resolver();

 private:
  Delegate* delegate_;

  DevToolsNetworkController* controller_;
  NetLog* net_log_;
  base::FilePath base_path_;
  bool in_memory_;
  base::MessageLoop* io_loop_;
  base::MessageLoop* file_loop_;

  scoped_ptr<net::ProxyConfigService> proxy_config_service_;
  scoped_ptr<net::NetworkDelegate> network_delegate_;
  scoped_ptr<net::URLRequestContextStorage> storage_;
  scoped_ptr<net::URLRequestContext> url_request_context_;
  scoped_ptr<net::HostMappingRules> host_mapping_rules_;
  scoped_ptr<ExplicitURLSecurityManager> url_sec_mgr_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector protocol_interceptors_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestContextGetter);
};

}  // namespace brightray

#endif
