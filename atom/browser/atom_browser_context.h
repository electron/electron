// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include <string>

#include "brightray/browser/browser_context.h"

namespace atom {

class AtomDownloadManagerDelegate;
class AtomCertVerifier;
class AtomNetworkDelegate;
class AtomPermissionManager;
class WebViewManager;

class AtomBrowserContext : public brightray::BrowserContext {
 public:
  AtomBrowserContext(const std::string& partition, bool in_memory);
  ~AtomBrowserContext() override;

  // brightray::URLRequestContextGetter::Delegate:
  net::NetworkDelegate* CreateNetworkDelegate() override;
  std::string GetUserAgent() override;
  std::unique_ptr<net::URLRequestJobFactory> CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* protocol_handlers) override;
  net::HttpCache::BackendFactory* CreateHttpCacheBackendFactory(
      const base::FilePath& base_path) override;
  std::unique_ptr<net::CertVerifier> CreateCertVerifier() override;
  net::SSLConfigService* CreateSSLConfigService() override;

  // content::BrowserContext:
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  content::PermissionManager* GetPermissionManager() override;

  // brightray::BrowserContext:
  void RegisterPrefs(PrefRegistrySimple* pref_registry) override;

  AtomCertVerifier* cert_verifier() const { return cert_verifier_; }
  AtomNetworkDelegate* network_delegate() const { return network_delegate_; }

 private:
  std::unique_ptr<AtomDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<WebViewManager> guest_manager_;
  std::unique_ptr<AtomPermissionManager> permission_manager_;

  // Managed by brightray::BrowserContext.
  AtomCertVerifier* cert_verifier_;
  AtomNetworkDelegate* network_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
