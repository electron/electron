// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_NETWORK_CONTEXT_SERVICE_H_
#define ELECTRON_SHELL_BROWSER_NET_NETWORK_CONTEXT_SERVICE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/net/proxy_config_monitor.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/net_buildflags.h"
#include "services/cert_verifier/public/mojom/cert_verifier_service_factory.mojom-forward.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"

class CookieEncryptionProviderImpl;

namespace base {
class FilePath;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace unexportable_keys {
class UnexportableKeyService;
class UnexportableKeyServiceProxyImpl;
}  // namespace unexportable_keys

namespace electron {

class ElectronBrowserContext;

// KeyedService that initializes and provides access to the NetworkContexts for
// a BrowserContext.
class NetworkContextService : public KeyedService {
 public:
  explicit NetworkContextService(content::BrowserContext* context);
  ~NetworkContextService() override;

  NetworkContextService(const NetworkContextService&) = delete;
  NetworkContextService& operator=(const NetworkContextService&) = delete;

  void ConfigureNetworkContextParams(
      network::mojom::NetworkContextParams* network_context_params,
      cert_verifier::mojom::CertVerifierCreationParams*
          cert_verifier_creation_params);

 private:
  // Creates parameters for the NetworkContext.
  network::mojom::NetworkContextParamsPtr CreateNetworkContextParams(
      bool in_memory,
      const base::FilePath& path);

  raw_ptr<ElectronBrowserContext> browser_context_;
  ProxyConfigMonitor proxy_config_monitor_;
  std::unique_ptr<CookieEncryptionProviderImpl> cookie_encryption_provider_;
#if BUILDFLAG(ENABLE_DEVICE_BOUND_SESSIONS)
  // This context's DBSC signing keys. Declared before the proxy, which holds a
  // bare pointer to it, so that the proxy is destroyed first.
  std::unique_ptr<unexportable_keys::UnexportableKeyService>
      unexportable_key_service_;
  // Serves the network service's calls into `unexportable_key_service_`.
  std::unique_ptr<unexportable_keys::UnexportableKeyServiceProxyImpl>
      unexportable_key_service_proxy_;
#endif
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_NETWORK_CONTEXT_SERVICE_H_
