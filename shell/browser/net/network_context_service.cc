// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/network_context_service.h"

#include "chrome/common/chrome_constants.h"
#include "content/public/browser/network_service_instance.h"
#include "services/network/network_service.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/net/system_network_context_manager.h"

namespace electron {

NetworkContextService::NetworkContextService(content::BrowserContext* context)
    : browser_context_(static_cast<ElectronBrowserContext*>(context)),
      proxy_config_monitor_(browser_context_->prefs()) {}

NetworkContextService::~NetworkContextService() = default;

void NetworkContextService::ConfigureNetworkContextParams(
    network::mojom::NetworkContextParams* network_context_params,
    network::mojom::CertVerifierCreationParams* cert_verifier_creation_params) {
  bool in_memory = browser_context_->IsOffTheRecord();
  const base::FilePath& path = browser_context_->GetPath();

  g_browser_process->system_network_context_manager()
      ->ConfigureDefaultNetworkContextParams(network_context_params);

  network_context_params->user_agent = browser_context_->GetUserAgent();

  network_context_params->accept_language =
      net::HttpUtil::GenerateAcceptLanguageHeader(
          ElectronBrowserClient::Get()->GetApplicationLocale());

  // Enable the HTTP cache.
  network_context_params->http_cache_enabled =
      browser_context_->CanUseHttpCache();

  network_context_params->cookie_manager_params =
      network::mojom::CookieManagerParams::New();

  // Configure on-disk storage for persistent sessions.
  if (!in_memory) {
    // Configure the HTTP cache path and size.
    network_context_params->http_cache_path =
        path.Append(chrome::kCacheDirname);
    network_context_params->http_cache_max_size =
        browser_context_->GetMaxCacheSize();

    // Currently this just contains HttpServerProperties
    network_context_params->http_server_properties_path =
        path.Append(chrome::kNetworkPersistentStateFilename);

    // Configure persistent cookie path.
    network_context_params->cookie_path = path.Append(chrome::kCookieFilename);

    network_context_params->restore_old_session_cookies = false;
    network_context_params->persist_session_cookies = false;

    // TODO(deepak1556): Matches the existing behavior https://git.io/fxHMl,
    // enable encryption as a followup.
    network_context_params->enable_encrypted_cookies = false;

    network_context_params->transport_security_persister_path = path;
  }

#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  network_context_params->enable_ftp_url_support = true;
#endif  // !BUILDFLAG(DISABLE_FTP_SUPPORT)

  proxy_config_monitor_.AddToNetworkContextParams(network_context_params);

  BrowserProcessImpl::ApplyProxyModeFromCommandLine(
      browser_context_->in_memory_pref_store());
}

}  // namespace electron
