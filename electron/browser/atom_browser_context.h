// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_ELECTRON_BROWSER_CONTEXT_H_
#define ELECTRON_BROWSER_ELECTRON_BROWSER_CONTEXT_H_

#include <string>

#include "brightray/browser/browser_context.h"

namespace electron {

class ElectronDownloadManagerDelegate;
class ElectronCertVerifier;
class ElectronNetworkDelegate;
class ElectronPermissionManager;
class ElectronURLRequestJobFactory;
class WebViewManager;

class ElectronBrowserContext : public brightray::BrowserContext {
 public:
  ElectronBrowserContext(const std::string& partition, bool in_memory);
  ~ElectronBrowserContext() override;

  // brightray::URLRequestContextGetter::Delegate:
  net::NetworkDelegate* CreateNetworkDelegate() override;
  std::string GetUserAgent() override;
  scoped_ptr<net::URLRequestJobFactory> CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* handlers,
      content::URLRequestInterceptorScopedVector* interceptors) override;
  net::HttpCache::BackendFactory* CreateHttpCacheBackendFactory(
      const base::FilePath& base_path) override;
  scoped_ptr<net::CertVerifier> CreateCertVerifier() override;
  net::SSLConfigService* CreateSSLConfigService() override;
  bool AllowNTLMCredentialsForDomain(const GURL& auth_origin) override;

  // content::BrowserContext:
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  content::PermissionManager* GetPermissionManager() override;

  // brightray::BrowserContext:
  void RegisterPrefs(PrefRegistrySimple* pref_registry) override;

  void AllowNTLMCredentialsForAllDomains(bool should_allow);

  ElectronCertVerifier* cert_verifier() const { return cert_verifier_; }

  ElectronURLRequestJobFactory* job_factory() const { return job_factory_; }

  ElectronNetworkDelegate* network_delegate() const { return network_delegate_; }

 private:
  scoped_ptr<ElectronDownloadManagerDelegate> download_manager_delegate_;
  scoped_ptr<WebViewManager> guest_manager_;
  scoped_ptr<ElectronPermissionManager> permission_manager_;

  // Managed by brightray::BrowserContext.
  ElectronCertVerifier* cert_verifier_;
  ElectronURLRequestJobFactory* job_factory_;
  ElectronNetworkDelegate* network_delegate_;

  bool allow_ntlm_everywhere_;

  DISALLOW_COPY_AND_ASSIGN(ElectronBrowserContext);
};

}  // namespace electron

#endif  // ELECTRON_BROWSER_ELECTRON_BROWSER_CONTEXT_H_
