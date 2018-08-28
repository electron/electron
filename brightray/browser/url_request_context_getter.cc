// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/url_request_context_getter.h"

#include <algorithm>

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "brightray/browser/browser_client.h"
#include "brightray/browser/browser_context.h"
#include "brightray/browser/net/require_ct_delegate.h"
#include "brightray/browser/net_log.h"
#include "brightray/common/switches.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/browser/devtools_network_transaction_factory.h"
#include "content/public/browser/resource_context.h"
#include "net/base/host_mapping_rules.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_known_logs.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_store.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/extras/sqlite/sqlite_channel_id_store.h"
#include "net/http/http_auth_filter.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_server_properties_impl.h"
#include "net/log/net_log.h"
#include "net/proxy_resolution/dhcp_pac_file_fetcher_factory.h"
#include "net/proxy_resolution/pac_file_fetcher_impl.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_service.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "services/network/public/cpp/network_switches.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "url/url_constants.h"

using content::BrowserThread;

namespace brightray {

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
    base::WeakPtr<BrowserContext> browser_context)
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
      BrowserClient::Get()->GetNetLog(), resource_context_.get(),
      browser_context_->IsOffTheRecord(), browser_context_->GetUserAgent(),
      browser_context_->GetPath(), protocol_handlers,
      std::move(protocol_interceptors));
  browser_context_->OnMainRequestContextCreated(
      main_request_context_getter_.get());
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
    NetLog* net_log,
    ResourceContext* resource_context,
    bool in_memory,
    const std::string& user_agent,
    const base::FilePath& base_path,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors)
    : job_factory_(nullptr),
      delegate_(nullptr),
      net_log_(net_log),
      resource_context_(resource_context),
      protocol_interceptors_(std::move(protocol_interceptors)),
      base_path_(base_path),
      in_memory_(in_memory),
      user_agent_(user_agent),
      context_shutting_down_(false) {
  // Must first be created on the UI thread.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (protocol_handlers)
    std::swap(protocol_handlers_, *protocol_handlers);

  // We must create the proxy config service on the UI loop on Linux because it
  // must synchronously run on the glib message loop. This will be passed to
  // the URLRequestContextStorage on the IO thread in GetURLRequestContext().
  proxy_config_service_ =
      net::ProxyResolutionService::CreateSystemProxyConfigService(
          BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
}

URLRequestContextGetter::~URLRequestContextGetter() {}

void URLRequestContextGetter::NotifyContextShuttingDown(
    std::unique_ptr<ResourceContext> resource_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  context_shutting_down_ = true;
  cookie_change_sub_.reset();
  resource_context.reset();
  net::URLRequestContextGetter::NotifyContextShuttingDown();
  url_request_context_.reset();
  storage_.reset();
  http_network_session_.reset();
  http_auth_preferences_.reset();
  host_mapping_rules_.reset();
  ct_delegate_.reset();
}

net::URLRequestContext* URLRequestContextGetter::GetURLRequestContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (context_shutting_down_)
    return nullptr;

  if (!url_request_context_.get()) {
    ct_delegate_.reset(new RequireCTDelegate);
    auto& command_line = *base::CommandLine::ForCurrentProcess();
    url_request_context_.reset(new net::URLRequestContext);

    // --log-net-log
    if (net_log_) {
      net_log_->StartLogging();
      url_request_context_->set_net_log(net_log_);
    }

    storage_.reset(
        new net::URLRequestContextStorage(url_request_context_.get()));

    storage_->set_network_delegate(delegate_->CreateNetworkDelegate());

    std::unique_ptr<net::CookieStore> cookie_store;
    scoped_refptr<net::SQLiteChannelIDStore> channel_id_db;
    // Create a single task runner to use with the CookieStore and
    // ChannelIDStore.
    scoped_refptr<base::SequencedTaskRunner> cookie_background_task_runner =
        base::CreateSequencedTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::BACKGROUND,
             base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
    auto cookie_path = in_memory_
                           ? base::FilePath()
                           : base_path_.Append(FILE_PATH_LITERAL("Cookies"));
    if (!in_memory_) {
      channel_id_db = new net::SQLiteChannelIDStore(
          base_path_.Append(FILE_PATH_LITERAL("Origin Bound Certs")),
          cookie_background_task_runner);
    }
    std::unique_ptr<net::ChannelIDService> channel_id_service(
        new net::ChannelIDService(
            new net::DefaultChannelIDStore(channel_id_db.get())));
    content::CookieStoreConfig cookie_config(cookie_path, false, false,
                                             nullptr);
    cookie_config.channel_id_service = channel_id_service.get();
    cookie_config.background_task_runner = cookie_background_task_runner;
    cookie_store = content::CreateCookieStore(cookie_config);
    cookie_store->SetChannelIDServiceID(channel_id_service->GetUniqueID());

    // Set custom schemes that can accept cookies.
    net::CookieMonster* cookie_monster =
        static_cast<net::CookieMonster*>(cookie_store.get());
    std::vector<std::string> cookie_schemes({"http", "https", "ws", "wss"});
    delegate_->GetCookieableSchemes(&cookie_schemes);
    cookie_monster->SetCookieableSchemes(cookie_schemes);
    // Cookie store will outlive notifier by order of declaration
    // in the header.
    cookie_change_sub_ =
        cookie_store->GetChangeDispatcher().AddCallbackForAllChanges(
            base::Bind(&URLRequestContextGetter::OnCookieChanged,
                       base::RetainedRef(this)));
    storage_->set_cookie_store(std::move(cookie_store));
    storage_->set_channel_id_service(std::move(channel_id_service));

    storage_->set_http_user_agent_settings(
        base::WrapUnique(new net::StaticHttpUserAgentSettings(
            net::HttpUtil::GenerateAcceptLanguageHeader(
                BrowserClient::Get()->GetApplicationLocale()),
            user_agent_)));

    std::unique_ptr<net::HostResolver> host_resolver(
        net::HostResolver::CreateDefaultResolver(nullptr));

    // --host-resolver-rules
    if (command_line.HasSwitch(network::switches::kHostResolverRules)) {
      std::unique_ptr<net::MappedHostResolver> remapped_resolver(
          new net::MappedHostResolver(std::move(host_resolver)));
      remapped_resolver->SetRulesFromString(command_line.GetSwitchValueASCII(
          network::switches::kHostResolverRules));
      host_resolver = std::move(remapped_resolver);
    }

    // --proxy-server
    if (command_line.HasSwitch(switches::kNoProxyServer)) {
      storage_->set_proxy_resolution_service(
          net::ProxyResolutionService::CreateDirect());
    } else if (command_line.HasSwitch(switches::kProxyServer)) {
      net::ProxyConfig proxy_config;
      proxy_config.proxy_rules().ParseFromString(
          command_line.GetSwitchValueASCII(switches::kProxyServer));
      proxy_config.proxy_rules().bypass_rules.ParseFromString(
          command_line.GetSwitchValueASCII(switches::kProxyBypassList));
      storage_->set_proxy_resolution_service(
          net::ProxyResolutionService::CreateFixed(proxy_config));
    } else if (command_line.HasSwitch(switches::kProxyPacUrl)) {
      auto proxy_config = net::ProxyConfig::CreateFromCustomPacURL(
          GURL(command_line.GetSwitchValueASCII(switches::kProxyPacUrl)));
      proxy_config.set_pac_mandatory(true);
      storage_->set_proxy_resolution_service(
          net::ProxyResolutionService::CreateFixed(proxy_config));
    } else {
      storage_->set_proxy_resolution_service(
          net::ProxyResolutionService::CreateUsingSystemProxyResolver(
              std::move(proxy_config_service_), net_log_));
    }

    std::vector<std::string> schemes;
    schemes.push_back(std::string("basic"));
    schemes.push_back(std::string("digest"));
    schemes.push_back(std::string("ntlm"));
    schemes.push_back(std::string("negotiate"));
#if defined(OS_POSIX)
    http_auth_preferences_.reset(
        new net::HttpAuthPreferences(schemes, std::string()));
#else
    http_auth_preferences_.reset(new net::HttpAuthPreferences(schemes));
#endif

    // --auth-server-whitelist
    if (command_line.HasSwitch(switches::kAuthServerWhitelist)) {
      http_auth_preferences_->SetServerWhitelist(
          command_line.GetSwitchValueASCII(switches::kAuthServerWhitelist));
    }

    // --auth-negotiate-delegate-whitelist
    if (command_line.HasSwitch(switches::kAuthNegotiateDelegateWhitelist)) {
      http_auth_preferences_->SetDelegateWhitelist(
          command_line.GetSwitchValueASCII(
              switches::kAuthNegotiateDelegateWhitelist));
    }

    auto auth_handler_factory = net::HttpAuthHandlerRegistryFactory::Create(
        http_auth_preferences_.get(), host_resolver.get());

    std::unique_ptr<net::TransportSecurityState> transport_security_state =
        base::WrapUnique(new net::TransportSecurityState);
    transport_security_state->SetRequireCTDelegate(ct_delegate_.get());
    storage_->set_transport_security_state(std::move(transport_security_state));
    storage_->set_cert_verifier(
        delegate_->CreateCertVerifier(ct_delegate_.get()));
    storage_->set_ssl_config_service(new net::SSLConfigServiceDefaults());
    storage_->set_http_auth_handler_factory(std::move(auth_handler_factory));
    std::unique_ptr<net::HttpServerProperties> server_properties(
        new net::HttpServerPropertiesImpl);
    storage_->set_http_server_properties(std::move(server_properties));

    std::unique_ptr<net::MultiLogCTVerifier> ct_verifier =
        std::make_unique<net::MultiLogCTVerifier>();
    ct_verifier->AddLogs(net::ct::CreateLogVerifiersForKnownLogs());
    storage_->set_cert_transparency_verifier(std::move(ct_verifier));
    storage_->set_ct_policy_enforcer(std::make_unique<net::CTPolicyEnforcer>());

    net::HttpNetworkSession::Params network_session_params;
    network_session_params.ignore_certificate_errors = false;

    // --disable-http2
    if (command_line.HasSwitch(switches::kDisableHttp2))
      network_session_params.enable_http2 = false;

    // --ignore-certificate-errors
    if (command_line.HasSwitch(::switches::kIgnoreCertificateErrors))
      network_session_params.ignore_certificate_errors = true;

    // --host-rules
    if (command_line.HasSwitch(switches::kHostRules)) {
      host_mapping_rules_.reset(new net::HostMappingRules);
      host_mapping_rules_->SetRulesFromString(
          command_line.GetSwitchValueASCII(switches::kHostRules));
      network_session_params.host_mapping_rules = *host_mapping_rules_.get();
    }

    // Give |storage_| ownership at the end in case it's |mapped_host_resolver|.
    storage_->set_host_resolver(std::move(host_resolver));

    net::HttpNetworkSession::Context network_session_context;
    net::URLRequestContextBuilder::SetHttpNetworkSessionComponents(
        url_request_context_.get(), &network_session_context);
    http_network_session_.reset(new net::HttpNetworkSession(
        network_session_params, network_session_context));

    std::unique_ptr<net::HttpCache::BackendFactory> backend;
    if (in_memory_) {
      backend = net::HttpCache::DefaultBackend::InMemory(0);
    } else {
      backend.reset(delegate_->CreateHttpCacheBackendFactory(base_path_));
    }

    storage_->set_http_transaction_factory(std::make_unique<net::HttpCache>(
        content::CreateDevToolsNetworkTransactionFactory(
            http_network_session_.get()),
        std::move(backend), false));

    std::unique_ptr<net::URLRequestJobFactory> job_factory =
        delegate_->CreateURLRequestJobFactory(url_request_context_.get(),
                                              &protocol_handlers_);
    job_factory_ = job_factory.get();

    // Set up interceptors in the reverse order.
    std::unique_ptr<net::URLRequestJobFactory> top_job_factory =
        std::move(job_factory);
    if (!protocol_interceptors_.empty()) {
      for (auto it = protocol_interceptors_.rbegin();
           it != protocol_interceptors_.rend(); ++it) {
        top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
            std::move(top_job_factory), std::move(*it)));
      }
      protocol_interceptors_.clear();
    }

    storage_->set_job_factory(std::move(top_job_factory));
  }

  if (resource_context_)
    resource_context_->request_context_ = url_request_context_.get();

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
URLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
}

void URLRequestContextGetter::OnCookieChanged(
    const net::CanonicalCookie& cookie,
    net::CookieChangeCause cause) const {
  if (delegate_)
    delegate_->OnCookieChanged(cookie, cause);
}

}  // namespace brightray
