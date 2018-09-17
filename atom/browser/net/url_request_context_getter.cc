// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_context_getter.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_cert_verifier.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "brightray/browser/browser_client.h"
#include "brightray/browser/net/require_ct_delegate.h"
#include "brightray/browser/net_log.h"
#include "brightray/common/switches.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/browser/devtools_network_transaction_factory.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/resource_context.h"
#include "net/base/host_mapping_rules.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_known_logs.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cookies/cookie_monster.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_transaction_factory.h"
#include "net/log/net_log.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "services/network/ignore_errors_cert_verifier.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/network/url_request_context_builder_mojo.h"
#include "url/url_constants.h"

using content::BrowserThread;

namespace atom {

namespace {

network::mojom::NetworkContextParamsPtr CreateDefaultNetworkContextParams(
    const base::FilePath& base_path,
    const std::string& user_agent,
    bool in_memory,
    bool use_cache,
    int max_cache_size) {
  network::mojom::NetworkContextParamsPtr network_context_params =
      network::mojom::NetworkContextParams::New();
  network_context_params->enable_brotli = true;
  network_context_params->user_agent = user_agent;
  network_context_params->http_cache_enabled = use_cache;
  network_context_params->accept_language =
      net::HttpUtil::GenerateAcceptLanguageHeader(
          brightray::BrowserClient::Get()->GetApplicationLocale());
  if (!in_memory) {
    network_context_params->http_cache_path =
        base_path.Append(FILE_PATH_LITERAL("Cache"));
    network_context_params->http_cache_max_size = max_cache_size;
    network_context_params->http_server_properties_path =
        base_path.Append(FILE_PATH_LITERAL("Network Persistent State"));
    network_context_params->cookie_path =
        base_path.Append(FILE_PATH_LITERAL("Cookies"));
    network_context_params->channel_id_path =
        base_path.Append(FILE_PATH_LITERAL("Origin Bound Certs"));
    network_context_params->restore_old_session_cookies = false;
    network_context_params->persist_session_cookies = false;
  }
  network_context_params->enable_data_url_support = true;
  network_context_params->enable_file_url_support = true;
#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  network_context_params->enable_ftp_url_support = true;
#endif  // !BUILDFLAG(DISABLE_FTP_SUPPORT)
  return network_context_params;
}

}  // namespace

class ResourceContext : public content::ResourceContext {
 public:
  ResourceContext() = default;
  ~ResourceContext() override = default;

  net::HostResolver* GetHostResolver() override {
    if (request_context_)
      return request_context_->host_resolver();
    return nullptr;
  }

  net::URLRequestContext* GetRequestContext() override {
    return request_context_;
  }

 private:
  friend class URLRequestContextGetter;

  net::URLRequestContext* request_context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ResourceContext);
};

URLRequestContextGetter::Handle::Handle(
    base::WeakPtr<AtomBrowserContext> browser_context)
    : resource_context_(new ResourceContext),
      browser_context_(browser_context),
      initialized_(false) {}

URLRequestContextGetter::Handle::~Handle() {}

content::ResourceContext* URLRequestContextGetter::Handle::GetResourceContext()
    const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LazyInitialize();
  return resource_context_.get();
}

scoped_refptr<URLRequestContextGetter>
URLRequestContextGetter::Handle::CreateMainRequestContextGetter(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!main_request_context_getter_.get());
  main_request_context_getter_ = new URLRequestContextGetter(
      brightray::BrowserClient::Get()->GetNetLog(), this, protocol_handlers,
      std::move(protocol_interceptors));
  return main_request_context_getter_;
}

scoped_refptr<URLRequestContextGetter>
URLRequestContextGetter::Handle::GetMainRequestContextGetter() const {
  return main_request_context_getter_;
}

void URLRequestContextGetter::Handle::LazyInitialize() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (initialized_)
    return;

  initialized_ = true;
  main_network_context_params_ = CreateDefaultNetworkContextParams(
      browser_context_->GetPath(), browser_context_->GetUserAgent(),
      browser_context_->IsOffTheRecord(), browser_context_->CanUseHttpCache(),
      browser_context_->GetMaxCacheSize());
  content::BrowserContext::EnsureResourceContextInitialized(
      browser_context_.get());
}

void URLRequestContextGetter::Handle::ShutdownOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (main_request_context_getter_.get()) {
    if (BrowserThread::IsThreadInitialized(BrowserThread::IO)) {
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::BindOnce(&URLRequestContextGetter::NotifyContextShuttingDown,
                         base::RetainedRef(main_request_context_getter_),
                         std::move(resource_context_)));
    }
  }

  if (!BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE, this))
    delete this;
}

URLRequestContextGetter::URLRequestContextGetter(
    brightray::NetLog* net_log,
    URLRequestContextGetter::Handle* context_handle,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors)
    : context_handle_(context_handle),
      main_request_context_(nullptr),
      protocol_interceptors_(std::move(protocol_interceptors)),
      context_shutting_down_(false) {
  // Must first be created on the UI thread.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (protocol_handlers)
    std::swap(protocol_handlers_, *protocol_handlers);
}

URLRequestContextGetter::~URLRequestContextGetter() {}

void URLRequestContextGetter::NotifyContextShuttingDown(
    std::unique_ptr<ResourceContext> resource_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  context_shutting_down_ = true;
  resource_context.reset();
  net::URLRequestContextGetter::NotifyContextShuttingDown();
  main_network_context_.reset();
  ct_delegate_.reset();
}

net::URLRequestContext* URLRequestContextGetter::GetURLRequestContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (context_shutting_down_)
    return nullptr;

  if (!main_network_context_) {
    auto& command_line = *base::CommandLine::ForCurrentProcess();
    std::unique_ptr<network::URLRequestContextBuilderMojo> builder =
        std::make_unique<network::URLRequestContextBuilderMojo>();
    builder->set_network_delegate(std::make_unique<AtomNetworkDelegate>());

    ct_delegate_.reset(new brightray::RequireCTDelegate);
    std::unique_ptr<net::CertVerifier> cert_verifier =
        std::make_unique<AtomCertVerifier>(ct_delegate_.get());
    auto spki_list = base::SplitString(
        command_line.GetSwitchValueASCII(
            network::switches::kIgnoreCertificateErrorsSPKIList),
        ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    cert_verifier = std::make_unique<network::IgnoreErrorsCertVerifier>(
        std::move(cert_verifier),
        network::IgnoreErrorsCertVerifier::MakeWhitelist(spki_list));
    builder->SetCertVerifier(std::move(cert_verifier));

    for (auto& protocol_handler : protocol_handlers_) {
      builder->SetProtocolHandler(protocol_handler.first,
                                  std::move(protocol_handler.second));
    }
    protocol_handlers_.clear();
    builder->SetInterceptors(std::move(protocol_interceptors_));
    builder->SetCreateHttpTransactionFactoryCallback(
        base::BindOnce(&content::CreateDevToolsNetworkTransactionFactory));

    std::unique_ptr<net::HostResolver> host_resolver =
        net::HostResolver::CreateDefaultResolver(net_log_);
    // --host-resolver-rules
    if (command_line.HasSwitch(network::switches::kHostResolverRules)) {
      auto remapped_resolver =
          std::make_unique<net::MappedHostResolver>(std::move(host_resolver));
      remapped_resolver->SetRulesFromString(command_line.GetSwitchValueASCII(
          network::switches::kHostResolverRules));
      host_resolver = std::move(remapped_resolver);
    }

    net::HttpAuthPreferences auth_preferences;
    // --auth-server-whitelist
    if (command_line.HasSwitch(brightray::switches::kAuthServerWhitelist)) {
      auth_preferences.SetServerWhitelist(command_line.GetSwitchValueASCII(
          brightray::switches::kAuthServerWhitelist));
    }

    // --auth-negotiate-delegate-whitelist
    if (command_line.HasSwitch(
            brightray::switches::kAuthNegotiateDelegateWhitelist)) {
      auth_preferences.SetDelegateWhitelist(command_line.GetSwitchValueASCII(
          brightray::switches::kAuthNegotiateDelegateWhitelist));
    }
    auto http_auth_handler_factory =
        net::HttpAuthHandlerRegistryFactory::CreateDefault(host_resolver.get());
    http_auth_handler_factory->SetHttpAuthPreferences(net::kNegotiateAuthScheme,
                                                      &auth_preferences);
    builder->SetHttpAuthHandlerFactory(std::move(http_auth_handler_factory));
    builder->set_host_resolver(std::move(host_resolver));

    auto ct_verifier = std::make_unique<net::MultiLogCTVerifier>();
    // Add built-in logs
    ct_verifier->AddLogs(net::ct::CreateLogVerifiersForKnownLogs());
    builder->set_ct_verifier(std::move(ct_verifier));

    // --proxy-server
    if (command_line.HasSwitch(brightray::switches::kNoProxyServer)) {
      builder->set_proxy_resolution_service(
          net::ProxyResolutionService::CreateDirect());
    } else if (command_line.HasSwitch(brightray::switches::kProxyServer)) {
      net::ProxyConfig proxy_config;
      proxy_config.proxy_rules().ParseFromString(
          command_line.GetSwitchValueASCII(brightray::switches::kProxyServer));
      proxy_config.proxy_rules().bypass_rules.ParseFromString(
          command_line.GetSwitchValueASCII(
              brightray::switches::kProxyBypassList));
      builder->set_proxy_resolution_service(
          net::ProxyResolutionService::CreateFixed(
              net::ProxyConfigWithAnnotation(proxy_config,
                                             NO_TRAFFIC_ANNOTATION_YET)));
    } else if (command_line.HasSwitch(brightray::switches::kProxyPacUrl)) {
      auto proxy_config = net::ProxyConfig::CreateFromCustomPacURL(GURL(
          command_line.GetSwitchValueASCII(brightray::switches::kProxyPacUrl)));
      proxy_config.set_pac_mandatory(true);
      builder->set_proxy_resolution_service(
          net::ProxyResolutionService::CreateFixed(
              net::ProxyConfigWithAnnotation(proxy_config,
                                             NO_TRAFFIC_ANNOTATION_YET)));
    }

    main_network_context_ =
        content::GetNetworkServiceImpl()->CreateNetworkContextWithBuilder(
            std::move(context_handle_->main_network_context_request_),
            std::move(context_handle_->main_network_context_params_),
            std::move(builder), &main_request_context_);

    net::TransportSecurityState* transport_security_state =
        main_request_context_->transport_security_state();
    transport_security_state->SetRequireCTDelegate(ct_delegate_.get());

    context_handle_->resource_context_->request_context_ =
        main_request_context_;
  }

  return main_request_context_;
}

scoped_refptr<base::SingleThreadTaskRunner>
URLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
}

}  // namespace atom
