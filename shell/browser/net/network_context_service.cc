// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/network_context_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind_helpers.h"
#include "chrome/common/chrome_constants.h"
#include "content/public/browser/network_service_instance.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/network_service.h"
#include "shell/browser/atom_browser_client.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/net/system_network_context_manager.h"

namespace electron {

NetworkContextService::NetworkContextService(content::BrowserContext* context)
    : browser_context_(static_cast<AtomBrowserContext*>(context)),
      proxy_config_monitor_(browser_context_->prefs()) {}

NetworkContextService::~NetworkContextService() = default;

network::mojom::NetworkContextPtr
NetworkContextService::CreateNetworkContext() {
  network::mojom::NetworkContextPtr network_context;

  content::GetNetworkService()->CreateNetworkContext(
      MakeRequest(&network_context),
      CreateNetworkContextParams(browser_context_->IsOffTheRecord(),
                                 browser_context_->GetPath()));

  return network_context;
}

class Foo : public network::mojom::CertVerifierClient {
 public:
  explicit Foo(AtomBrowserContext* browser_context)
      : browser_context_(browser_context) {}
  ~Foo() override = default;

  // network::mojom::CertVerifierClient
  void Verify(int default_error,
              const net::CertVerifyResult& default_result,
              const scoped_refptr<net::X509Certificate>& certificate,
              const std::string& hostname,
              int flags,
              const base::Optional<std::string>& ocsp_response,
              VerifyCallback callback) override {
    VerifyRequestParams params;
    params.hostname = hostname;
    params.default_result = net::ErrorToString(default_error);
    params.error_code = default_error;
    params.certificate = certificate;
    browser_context_->cert_verify_proc().Run(
        params,
        base::AdaptCallbackForRepeating(base::BindOnce(
            [](VerifyCallback callback, const net::CertVerifyResult& result,
               int err) { std::move(callback).Run(err, result); },
            std::move(callback), default_result)));
  }

 private:
  AtomBrowserContext* browser_context_;
};

network::mojom::NetworkContextParamsPtr
NetworkContextService::CreateNetworkContextParams(bool in_memory,
                                                  const base::FilePath& path) {
  network::mojom::NetworkContextParamsPtr network_context_params =
      g_browser_process->system_network_context_manager()
          ->CreateDefaultNetworkContextParams();

  network_context_params->user_agent = browser_context_->GetUserAgent();

  network_context_params->accept_language =
      net::HttpUtil::GenerateAcceptLanguageHeader(
          AtomBrowserClient::Get()->GetApplicationLocale());

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

  network::mojom::CertVerifierClientPtr cert_verifier_client;
  mojo::MakeStrongBinding(std::make_unique<Foo>(browser_context_),
                          mojo::MakeRequest(&cert_verifier_client));
  network_context_params->cert_verifier_client =
      cert_verifier_client.PassInterface();

#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  network_context_params->enable_ftp_url_support = true;
#endif  // !BUILDFLAG(DISABLE_FTP_SUPPORT)

  proxy_config_monitor_.AddToNetworkContextParams(network_context_params.get());

  BrowserProcessImpl::ApplyProxyModeFromCommandLine(
      browser_context_->in_memory_pref_store());

  return network_context_params;
}

}  // namespace electron
