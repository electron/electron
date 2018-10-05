// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_context_getter.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/about_protocol_handler.h"
#include "atom/browser/net/asar/asar_protocol_handler.h"
#include "atom/browser/net/atom_cert_verifier.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "atom/browser/net/http_protocol_handler.h"
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "brightray/browser/net/require_ct_delegate.h"
#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/value_map_pref_store.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_network_transaction_factory.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/content_switches.h"
#include "net/base/host_mapping_rules.h"
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
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "services/network/ignore_errors_cert_verifier.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/network/url_request_context_builder_mojo.h"
#include "url/url_constants.h"

#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
#include "net/url_request/ftp_protocol_handler.h"
#endif

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
  network_context_params->enable_data_url_support = false;
  network_context_params->proxy_resolver_factory =
      ChromeMojoProxyResolverFactory::CreateWithStrongBinding().PassInterface();
  if (!in_memory) {
    network_context_params->http_cache_path =
        base_path.Append(chrome::kCacheDirname);
    network_context_params->http_cache_max_size = max_cache_size;
    network_context_params->http_server_properties_path =
        base_path.Append(chrome::kNetworkPersistentStateFilename);
    network_context_params->cookie_path =
        base_path.Append(chrome::kCookieFilename);
    network_context_params->channel_id_path =
        base_path.Append(chrome::kChannelIDFilename);
    network_context_params->restore_old_session_cookies = false;
    network_context_params->persist_session_cookies = false;
  }
  // TODO(deepak1556): Decide the stand on chrome ct policy and
  // enable it.
  // See //net/docs/certificate-transparency.md
  // network_context_params->enforce_chrome_ct_policy = true;
  return network_context_params;
}

void SetupAtomURLRequestJobFactory(
    content::ProtocolHandlerMap* protocol_handlers,
    net::URLRequestContext* url_request_context,
    AtomURLRequestJobFactory* job_factory) {
  for (auto& protocol_handler : *protocol_handlers) {
    job_factory->SetProtocolHandler(protocol_handler.first,
                                    std::move(protocol_handler.second));
  }
  protocol_handlers->clear();

  job_factory->SetProtocolHandler(url::kAboutScheme,
                                  std::make_unique<AboutProtocolHandler>());
  job_factory->SetProtocolHandler(url::kDataScheme,
                                  std::make_unique<net::DataProtocolHandler>());
  job_factory->SetProtocolHandler(
      url::kFileScheme,
      std::make_unique<asar::AsarProtocolHandler>(
          base::CreateTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
               base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})));
  job_factory->SetProtocolHandler(
      url::kHttpScheme,
      std::make_unique<HttpProtocolHandler>(url::kHttpScheme));
  job_factory->SetProtocolHandler(
      url::kHttpsScheme,
      std::make_unique<HttpProtocolHandler>(url::kHttpsScheme));
  job_factory->SetProtocolHandler(
      url::kWsScheme, std::make_unique<HttpProtocolHandler>(url::kWsScheme));
  job_factory->SetProtocolHandler(
      url::kWssScheme, std::make_unique<HttpProtocolHandler>(url::kWssScheme));

#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  auto* host_resolver = url_request_context->host_resolver();
  job_factory->SetProtocolHandler(
      url::kFtpScheme, net::FtpProtocolHandler::Create(host_resolver));
#endif
}

void ApplyProxyModeFromCommandLine(ValueMapPrefStore* pref_store) {
  if (!pref_store)
    return;

  auto* command_line = base::CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(::switches::kNoProxyServer)) {
    pref_store->SetValue(proxy_config::prefs::kProxy,
                         ProxyConfigDictionary::CreateDirect(),
                         WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  } else if (command_line->HasSwitch(::switches::kProxyPacUrl)) {
    std::string pac_script_url =
        command_line->GetSwitchValueASCII(::switches::kProxyPacUrl);
    pref_store->SetValue(proxy_config::prefs::kProxy,
                         ProxyConfigDictionary::CreatePacScript(
                             pac_script_url, false /* pac_mandatory */),
                         WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  } else if (command_line->HasSwitch(::switches::kProxyAutoDetect)) {
    pref_store->SetValue(proxy_config::prefs::kProxy,
                         ProxyConfigDictionary::CreateAutoDetect(),
                         WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  } else if (command_line->HasSwitch(::switches::kProxyServer)) {
    std::string proxy_server =
        command_line->GetSwitchValueASCII(::switches::kProxyServer);
    std::string bypass_list =
        command_line->GetSwitchValueASCII(::switches::kProxyBypassList);
    pref_store->SetValue(
        proxy_config::prefs::kProxy,
        ProxyConfigDictionary::CreateFixedServers(proxy_server, bypass_list),
        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  }
}

}  // namespace

class ResourceContext : public content::ResourceContext {
 public:
  ResourceContext() = default;
  ~ResourceContext() override = default;

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

content::ResourceContext*
URLRequestContextGetter::Handle::GetResourceContext() {
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
  LazyInitialize();
  main_request_context_getter_ = new URLRequestContextGetter(
      this, protocol_handlers, std::move(protocol_interceptors));
  return main_request_context_getter_;
}

scoped_refptr<URLRequestContextGetter>
URLRequestContextGetter::Handle::GetMainRequestContextGetter() {
  return main_request_context_getter_;
}

network::mojom::NetworkContextPtr
URLRequestContextGetter::Handle::GetNetworkContext() {
  if (!main_network_context_) {
    main_network_context_request_ = mojo::MakeRequest(&main_network_context_);
  }
  return std::move(main_network_context_);
}

void URLRequestContextGetter::Handle::LazyInitialize() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (initialized_)
    return;

  initialized_ = true;
  main_network_context_params_ = CreateDefaultNetworkContextParams(
      browser_context_->GetPath(), browser_context_->GetUserAgent(),
      browser_context_->IsOffTheRecord(), browser_context_->CanUseHttpCache(),
      browser_context_->GetMaxCacheSize());

  browser_context_->proxy_config_monitor()->AddToNetworkContextParams(
      main_network_context_params_.get());

  ApplyProxyModeFromCommandLine(browser_context_->in_memory_pref_store());

  if (!main_network_context_request_.is_pending()) {
    main_network_context_request_ = mojo::MakeRequest(&main_network_context_);
  }
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
    URLRequestContextGetter::Handle* context_handle,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors)
    : context_handle_(context_handle),
      url_request_context_(nullptr),
      protocol_interceptors_(std::move(protocol_interceptors)),
      context_shutting_down_(false) {
  // Must first be created on the UI thread.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (protocol_handlers)
    std::swap(protocol_handlers_, *protocol_handlers);
}

URLRequestContextGetter::~URLRequestContextGetter() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // NotifyContextShuttingDown should have been called.
  DCHECK(context_shutting_down_);
}

void URLRequestContextGetter::NotifyContextShuttingDown(
    std::unique_ptr<ResourceContext> resource_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  context_shutting_down_ = true;
  resource_context.reset();
  net::URLRequestContextGetter::NotifyContextShuttingDown();
}

net::URLRequestContext* URLRequestContextGetter::GetURLRequestContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (context_shutting_down_)
    return nullptr;

  if (!url_request_context_) {
    std::unique_ptr<network::URLRequestContextBuilderMojo> builder =
        std::make_unique<network::URLRequestContextBuilderMojo>();
    builder->set_network_delegate(std::make_unique<AtomNetworkDelegate>());

    ct_delegate_.reset(new brightray::RequireCTDelegate);
    auto cert_verifier = std::make_unique<AtomCertVerifier>(ct_delegate_.get());
    builder->SetCertVerifier(std::move(cert_verifier));

    builder->SetCreateHttpTransactionFactoryCallback(
        base::BindOnce(&content::CreateDevToolsNetworkTransactionFactory));

    builder->set_ct_verifier(std::make_unique<net::MultiLogCTVerifier>());

    auto* network_service = content::GetNetworkServiceImpl();
    network_context_ = network_service->CreateNetworkContextWithBuilder(
        std::move(context_handle_->main_network_context_request_),
        std::move(context_handle_->main_network_context_params_),
        std::move(builder), &url_request_context_);

    net::TransportSecurityState* transport_security_state =
        url_request_context_->transport_security_state();
    transport_security_state->SetRequireCTDelegate(ct_delegate_.get());

    // Add custom standard schemes to cookie schemes.
    auto* cookie_monster =
        static_cast<net::CookieMonster*>(url_request_context_->cookie_store());
    std::vector<std::string> cookie_schemes(
        {url::kHttpScheme, url::kHttpsScheme, url::kWsScheme, url::kWssScheme});
    const auto& custom_standard_schemes = atom::api::GetStandardSchemes();
    cookie_schemes.insert(cookie_schemes.end(), custom_standard_schemes.begin(),
                          custom_standard_schemes.end());
    cookie_monster->SetCookieableSchemes(cookie_schemes);

    // Setup handlers for custom job factory.
    top_job_factory_.reset(new AtomURLRequestJobFactory);
    SetupAtomURLRequestJobFactory(&protocol_handlers_, url_request_context_,
                                  top_job_factory_.get());
    std::unique_ptr<net::URLRequestJobFactory> inner_job_factory(
        new net::URLRequestJobFactoryImpl);
    if (!protocol_interceptors_.empty()) {
      // Set up interceptors in the reverse order.
      for (auto it = protocol_interceptors_.rbegin();
           it != protocol_interceptors_.rend(); ++it) {
        inner_job_factory.reset(new net::URLRequestInterceptingJobFactory(
            std::move(inner_job_factory), std::move(*it)));
      }
      protocol_interceptors_.clear();
    }
    top_job_factory_->Chain(std::move(inner_job_factory));
    url_request_context_->set_job_factory(top_job_factory_.get());

    context_handle_->resource_context_->request_context_ = url_request_context_;
  }

  return url_request_context_;
}

scoped_refptr<base::SingleThreadTaskRunner>
URLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
}

}  // namespace atom
