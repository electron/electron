// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/system_network_context_manager.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_features.h"
#include "content/public/common/network_service_util.h"
#include "electron/fuses.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "net/dns/public/util.h"
#include "net/net_buildflags.h"
#include "services/cert_verifier/public/mojom/cert_verifier_service_factory.mojom.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/cross_thread_pending_shared_url_loader_factory.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "shell/browser/api/electron_api_safe_storage.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/common/application_info.h"
#include "shell/common/electron_paths.h"
#include "shell/common/options_switches.h"
#include "url/gurl.h"

#if defined(OS_MAC)
#include "components/os_crypt/keychain_password_mac.h"
#endif

#if defined(OS_LINUX)
#include "components/os_crypt/key_storage_config_linux.h"
#endif

namespace {

// The global instance of the SystemNetworkContextmanager.
SystemNetworkContextManager* g_system_network_context_manager = nullptr;

network::mojom::HttpAuthStaticParamsPtr CreateHttpAuthStaticParams() {
  network::mojom::HttpAuthStaticParamsPtr auth_static_params =
      network::mojom::HttpAuthStaticParams::New();

  auth_static_params->supported_schemes = {"basic", "digest", "ntlm",
                                           "negotiate"};

  return auth_static_params;
}

network::mojom::HttpAuthDynamicParamsPtr CreateHttpAuthDynamicParams() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  network::mojom::HttpAuthDynamicParamsPtr auth_dynamic_params =
      network::mojom::HttpAuthDynamicParams::New();

  auth_dynamic_params->server_allowlist = command_line->GetSwitchValueASCII(
      electron::switches::kAuthServerWhitelist);
  auth_dynamic_params->delegate_allowlist = command_line->GetSwitchValueASCII(
      electron::switches::kAuthNegotiateDelegateWhitelist);
  auth_dynamic_params->enable_negotiate_port =
      command_line->HasSwitch(electron::switches::kEnableAuthNegotiatePort);
  auth_dynamic_params->ntlm_v2_enabled =
      !command_line->HasSwitch(electron::switches::kDisableNTLMv2);

  return auth_dynamic_params;
}

}  // namespace

// SharedURLLoaderFactory backed by a SystemNetworkContextManager and its
// network context. Transparently handles crashes.
class SystemNetworkContextManager::URLLoaderFactoryForSystem
    : public network::SharedURLLoaderFactory {
 public:
  explicit URLLoaderFactoryForSystem(SystemNetworkContextManager* manager)
      : manager_(manager) {
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  // disable copy
  URLLoaderFactoryForSystem(const URLLoaderFactoryForSystem&) = delete;
  URLLoaderFactoryForSystem& operator=(const URLLoaderFactoryForSystem&) =
      delete;

  // mojom::URLLoaderFactory implementation:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> request,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& url_request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (!manager_)
      return;
    manager_->GetURLLoaderFactory()->CreateLoaderAndStart(
        std::move(request), request_id, options, url_request, std::move(client),
        traffic_annotation);
  }

  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver)
      override {
    if (!manager_)
      return;
    manager_->GetURLLoaderFactory()->Clone(std::move(receiver));
  }

  // SharedURLLoaderFactory implementation:
  std::unique_ptr<network::PendingSharedURLLoaderFactory> Clone() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    return std::make_unique<network::CrossThreadPendingSharedURLLoaderFactory>(
        this);
  }

  void Shutdown() { manager_ = nullptr; }

 private:
  friend class base::RefCounted<URLLoaderFactoryForSystem>;
  ~URLLoaderFactoryForSystem() override = default;

  SEQUENCE_CHECKER(sequence_checker_);
  SystemNetworkContextManager* manager_;
};

network::mojom::NetworkContext* SystemNetworkContextManager::GetContext() {
  if (!network_context_ || !network_context_.is_connected()) {
    // This should call into OnNetworkServiceCreated(), which will re-create
    // the network service, if needed. There's a chance that it won't be
    // invoked, if the NetworkContext has encountered an error but the
    // NetworkService has not yet noticed its pipe was closed. In that case,
    // trying to create a new NetworkContext would fail, anyways, and hopefully
    // a new NetworkContext will be created on the next GetContext() call.
    content::GetNetworkService();
    DCHECK(network_context_);
  }
  return network_context_.get();
}

network::mojom::URLLoaderFactory*
SystemNetworkContextManager::GetURLLoaderFactory() {
  // Create the URLLoaderFactory as needed.
  if (url_loader_factory_ && url_loader_factory_.is_connected()) {
    return url_loader_factory_.get();
  }

  network::mojom::URLLoaderFactoryParamsPtr params =
      network::mojom::URLLoaderFactoryParams::New();
  params->process_id = network::mojom::kBrowserProcessId;
  params->is_corb_enabled = false;
  url_loader_factory_.reset();
  GetContext()->CreateURLLoaderFactory(
      url_loader_factory_.BindNewPipeAndPassReceiver(), std::move(params));
  return url_loader_factory_.get();
}

scoped_refptr<network::SharedURLLoaderFactory>
SystemNetworkContextManager::GetSharedURLLoaderFactory() {
  return shared_url_loader_factory_;
}

network::mojom::NetworkContextParamsPtr
SystemNetworkContextManager::CreateDefaultNetworkContextParams() {
  network::mojom::NetworkContextParamsPtr network_context_params =
      network::mojom::NetworkContextParams::New();

  ConfigureDefaultNetworkContextParams(network_context_params.get());

  cert_verifier::mojom::CertVerifierCreationParamsPtr
      cert_verifier_creation_params =
          cert_verifier::mojom::CertVerifierCreationParams::New();
  network_context_params->cert_verifier_params =
      content::GetCertVerifierParams(std::move(cert_verifier_creation_params));
  return network_context_params;
}

void SystemNetworkContextManager::ConfigureDefaultNetworkContextParams(
    network::mojom::NetworkContextParams* network_context_params) {
  network_context_params->enable_brotli = true;

  network_context_params->enable_referrers = true;

  network_context_params->proxy_resolver_factory =
      ChromeMojoProxyResolverFactory::CreateWithSelfOwnedReceiver();
}

// static
SystemNetworkContextManager* SystemNetworkContextManager::CreateInstance(
    PrefService* pref_service) {
  DCHECK(!g_system_network_context_manager);
  g_system_network_context_manager =
      new SystemNetworkContextManager(pref_service);
  return g_system_network_context_manager;
}

// static
SystemNetworkContextManager* SystemNetworkContextManager::GetInstance() {
  return g_system_network_context_manager;
}

// static
void SystemNetworkContextManager::DeleteInstance() {
  DCHECK(g_system_network_context_manager);
  delete g_system_network_context_manager;
}

// c.f.
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/net/system_network_context_manager.cc;l=730-740;drc=15a616c8043551a7cb22c4f73a88e83afb94631c;bpv=1;bpt=1
bool SystemNetworkContextManager::IsNetworkSandboxEnabled() {
#if defined(OS_WIN)
  auto* local_state = g_browser_process->local_state();
  if (local_state &&
      local_state->HasPrefPath(prefs::kNetworkServiceSandboxEnabled)) {
    return local_state->GetBoolean(prefs::kNetworkServiceSandboxEnabled);
  }
#endif  // defined(OS_WIN)
  // If no policy is specified, then delegate to global sandbox configuration.
  return sandbox::policy::features::IsNetworkSandboxEnabled();
}

SystemNetworkContextManager::SystemNetworkContextManager(
    PrefService* pref_service)
    : proxy_config_monitor_(pref_service) {
  shared_url_loader_factory_ =
      base::MakeRefCounted<URLLoaderFactoryForSystem>(this);
}

SystemNetworkContextManager::~SystemNetworkContextManager() {
  shared_url_loader_factory_->Shutdown();
}

void SystemNetworkContextManager::OnNetworkServiceCreated(
    network::mojom::NetworkService* network_service) {
  network_service->SetUpHttpAuth(CreateHttpAuthStaticParams());
  network_service->ConfigureHttpAuthPrefs(CreateHttpAuthDynamicParams());

  network_context_.reset();
  network_service->CreateNetworkContext(
      network_context_.BindNewPipeAndPassReceiver(),
      CreateNetworkContextParams());

  net::SecureDnsMode default_secure_dns_mode = net::SecureDnsMode::kOff;
  std::string default_doh_templates;
  if (base::FeatureList::IsEnabled(features::kDnsOverHttps)) {
    if (features::kDnsOverHttpsFallbackParam.Get()) {
      default_secure_dns_mode = net::SecureDnsMode::kAutomatic;
    } else {
      default_secure_dns_mode = net::SecureDnsMode::kSecure;
    }
    default_doh_templates = features::kDnsOverHttpsTemplatesParam.Get();
  }
  std::string server_method;
  absl::optional<std::vector<network::mojom::DnsOverHttpsServerPtr>>
      servers_mojo;
  if (!default_doh_templates.empty() &&
      default_secure_dns_mode != net::SecureDnsMode::kOff) {
    for (base::StringPiece server_template :
         SplitStringPiece(default_doh_templates, " ", base::TRIM_WHITESPACE,
                          base::SPLIT_WANT_NONEMPTY)) {
      if (!net::dns_util::IsValidDohTemplate(server_template, &server_method)) {
        continue;
      }

      bool use_post = server_method == "POST";

      if (!servers_mojo.has_value()) {
        servers_mojo = absl::make_optional<
            std::vector<network::mojom::DnsOverHttpsServerPtr>>();
      }

      network::mojom::DnsOverHttpsServerPtr server_mojo =
          network::mojom::DnsOverHttpsServer::New();
      server_mojo->server_template = std::string(server_template);
      server_mojo->use_post = use_post;
      servers_mojo->emplace_back(std::move(server_mojo));
    }
  }

  bool additional_dns_query_types_enabled = true;

  // Configure the stub resolver. This must be done after the system
  // NetworkContext is created, but before anything has the chance to use it.
  content::GetNetworkService()->ConfigureStubHostResolver(
      base::FeatureList::IsEnabled(features::kAsyncDns),
      default_secure_dns_mode, std::move(servers_mojo),
      additional_dns_query_types_enabled);

  std::string app_name = electron::Browser::Get()->GetName();
#if defined(OS_MAC)
  KeychainPassword::GetServiceName() = app_name + " Safe Storage";
  KeychainPassword::GetAccountName() = app_name;
#endif
#if defined(OS_LINUX)
  // c.f.
  // https://source.chromium.org/chromium/chromium/src/+/master:chrome/browser/net/system_network_context_manager.cc;l=515;drc=9d82515060b9b75fa941986f5db7390299669ef1;bpv=1;bpt=1
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  auto config = std::make_unique<os_crypt::Config>();
  config->store = command_line.GetSwitchValueASCII(::switches::kPasswordStore);
  config->product_name = app_name;
  config->application_name = app_name;
  config->main_thread_runner = base::ThreadTaskRunnerHandle::Get();
  // c.f.
  // https://source.chromium.org/chromium/chromium/src/+/master:chrome/common/chrome_switches.cc;l=689;drc=9d82515060b9b75fa941986f5db7390299669ef1
  config->should_use_preference =
      command_line.HasSwitch(::switches::kEnableEncryptionSelection);
  base::PathService::Get(chrome::DIR_USER_DATA, &config->user_data_path);
#endif

  // The OSCrypt keys are process bound, so if network service is out of
  // process, send it the required key.
  if (content::IsOutOfProcessNetworkService() &&
      electron::fuses::IsCookieEncryptionEnabled()) {
#if defined(OS_LINUX)
    network::mojom::CryptConfigPtr network_crypt_config =
        network::mojom::CryptConfig::New();
    network_crypt_config->application_name = config->application_name;
    network_crypt_config->product_name = config->product_name;
    network_crypt_config->store = config->store;
    network_crypt_config->should_use_preference = config->should_use_preference;
    network_crypt_config->user_data_path = config->user_data_path;

    network_service->SetCryptConfig(std::move(network_crypt_config));

#else
    network_service->SetEncryptionKey(OSCrypt::GetRawEncryptionKey());
#endif
  }

#if defined(OS_LINUX)
  OSCrypt::SetConfig(std::move(config));
#endif

#if DCHECK_IS_ON()
  electron::safestorage::SetElectronCryptoReady(true);
#endif
}

network::mojom::NetworkContextParamsPtr
SystemNetworkContextManager::CreateNetworkContextParams() {
  // TODO(mmenke): Set up parameters here (in memory cookie store, etc).
  network::mojom::NetworkContextParamsPtr network_context_params =
      CreateDefaultNetworkContextParams();

  network_context_params->user_agent =
      electron::ElectronBrowserClient::Get()->GetUserAgent();

  network_context_params->http_cache_enabled = false;

  auto ssl_config = network::mojom::SSLConfig::New();
  ssl_config->version_min = network::mojom::SSLVersion::kTLS12;
  network_context_params->initial_ssl_config = std::move(ssl_config);

  proxy_config_monitor_.AddToNetworkContextParams(network_context_params.get());

  return network_context_params;
}
