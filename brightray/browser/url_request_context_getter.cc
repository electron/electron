// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/url_request_context_getter.h"

#include <algorithm>

#include "browser/net/devtools_network_controller_handle.h"
#include "browser/net/devtools_network_transaction_factory.h"
#include "browser/net_log.h"
#include "browser/network_delegate.h"
#include "common/switches.h"

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/common/content_switches.h"
#include "net/base/host_mapping_rules.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_known_logs.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cookies/cookie_monster.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/http/http_auth_filter.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_server_properties_impl.h"
#include "net/log/net_log.h"
#include "net/proxy/dhcp_proxy_script_fetcher_factory.h"
#include "net/proxy/proxy_config.h"
#include "net/proxy/proxy_config_service.h"
#include "net/proxy/proxy_script_fetcher_impl.h"
#include "net/proxy/proxy_service.h"
#include "net/proxy/proxy_service_v8.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_constants.h"
#include "storage/browser/quota/special_storage_policy.h"

#if defined(USE_NSS_CERTS)
#include "net/cert_net/nss_ocsp.h"
#endif

using content::BrowserThread;

namespace brightray {

std::string URLRequestContextGetter::Delegate::GetUserAgent() {
  return base::EmptyString();
}

std::unique_ptr<net::URLRequestJobFactory>
URLRequestContextGetter::Delegate::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* protocol_handlers) {
  std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(new net::URLRequestJobFactoryImpl);

  for (auto& it : *protocol_handlers) {
    job_factory->SetProtocolHandler(
        it.first, base::WrapUnique(it.second.release()));
  }
  protocol_handlers->clear();

  job_factory->SetProtocolHandler(
      url::kDataScheme, base::WrapUnique(new net::DataProtocolHandler));
  job_factory->SetProtocolHandler(
      url::kFileScheme,
      base::WrapUnique(new net::FileProtocolHandler(
          BrowserThread::GetBlockingPool()->GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::SKIP_ON_SHUTDOWN))));

  return std::move(job_factory);
}

net::HttpCache::BackendFactory*
URLRequestContextGetter::Delegate::CreateHttpCacheBackendFactory(const base::FilePath& base_path) {
  base::FilePath cache_path = base_path.Append(FILE_PATH_LITERAL("Cache"));
  return new net::HttpCache::DefaultBackend(
      net::DISK_CACHE,
      net::CACHE_BACKEND_DEFAULT,
      cache_path,
      0,
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE));
}

std::unique_ptr<net::CertVerifier>
URLRequestContextGetter::Delegate::CreateCertVerifier() {
  return net::CertVerifier::CreateDefault();
}

net::SSLConfigService* URLRequestContextGetter::Delegate::CreateSSLConfigService() {
  return new net::SSLConfigServiceDefaults;
}

std::vector<std::string> URLRequestContextGetter::Delegate::GetCookieableSchemes() {
  return { "http", "https", "ws", "wss" };
}

URLRequestContextGetter::URLRequestContextGetter(
    Delegate* delegate,
    DevToolsNetworkControllerHandle* handle,
    NetLog* net_log,
    const base::FilePath& base_path,
    bool in_memory,
    base::MessageLoop* io_loop,
    base::MessageLoop* file_loop,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors)
    : delegate_(delegate),
      network_controller_handle_(handle),
      net_log_(net_log),
      base_path_(base_path),
      in_memory_(in_memory),
      io_loop_(io_loop),
      file_loop_(file_loop),
      protocol_interceptors_(std::move(protocol_interceptors)),
      job_factory_(nullptr) {
  // Must first be created on the UI thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (protocol_handlers)
    std::swap(protocol_handlers_, *protocol_handlers);

  if (delegate_)
    user_agent_ = delegate_->GetUserAgent();

  // We must create the proxy config service on the UI loop on Linux because it
  // must synchronously run on the glib message loop. This will be passed to
  // the URLRequestContextStorage on the IO thread in GetURLRequestContext().
  proxy_config_service_ = net::ProxyService::CreateSystemProxyConfigService(
      io_loop_->task_runner(), file_loop_->task_runner());
}

URLRequestContextGetter::~URLRequestContextGetter() {
#if defined(USE_NSS_CERTS)
  net::SetURLRequestContextForNSSHttpIO(NULL);
#endif
}

net::HostResolver* URLRequestContextGetter::host_resolver() {
  return url_request_context_->host_resolver();
}

net::URLRequestContext* URLRequestContextGetter::GetURLRequestContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!url_request_context_.get()) {
    auto& command_line = *base::CommandLine::ForCurrentProcess();
    url_request_context_.reset(new net::URLRequestContext);

#if defined(USE_NSS_CERTS)
    net::SetURLRequestContextForNSSHttpIO(url_request_context_.get());
#endif

    // --log-net-log
    if (net_log_) {
      net_log_->StartLogging(url_request_context_.get());
      url_request_context_->set_net_log(net_log_);
    }

    network_delegate_.reset(delegate_->CreateNetworkDelegate());
    url_request_context_->set_network_delegate(network_delegate_.get());

    storage_.reset(new net::URLRequestContextStorage(url_request_context_.get()));

    std::unique_ptr<net::CookieStore> cookie_store = nullptr;
    if (in_memory_) {
      auto cookie_config = content::CookieStoreConfig();
      cookie_config.cookieable_schemes = delegate_->GetCookieableSchemes();
      cookie_store = content::CreateCookieStore(cookie_config);
    } else {
      auto cookie_config = content::CookieStoreConfig(
          base_path_.Append(FILE_PATH_LITERAL("Cookies")),
          content::CookieStoreConfig::EPHEMERAL_SESSION_COOKIES,
          nullptr, nullptr);
      cookie_config.cookieable_schemes = delegate_->GetCookieableSchemes();
      cookie_store = content::CreateCookieStore(cookie_config);
    }
    storage_->set_cookie_store(std::move(cookie_store));
    storage_->set_channel_id_service(base::WrapUnique(
        new net::ChannelIDService(new net::DefaultChannelIDStore(nullptr),
                                  base::WorkerPool::GetTaskRunner(true))));

    std::string accept_lang = l10n_util::GetApplicationLocale("");
    storage_->set_http_user_agent_settings(base::WrapUnique(
        new net::StaticHttpUserAgentSettings(
            net::HttpUtil::GenerateAcceptLanguageHeader(accept_lang),
            user_agent_)));

    std::unique_ptr<net::HostResolver> host_resolver(net::HostResolver::CreateDefaultResolver(nullptr));

    // --host-resolver-rules
    if (command_line.HasSwitch(::switches::kHostResolverRules)) {
      std::unique_ptr<net::MappedHostResolver> remapped_resolver(
          new net::MappedHostResolver(std::move(host_resolver)));
      remapped_resolver->SetRulesFromString(
          command_line.GetSwitchValueASCII(::switches::kHostResolverRules));
      host_resolver = std::move(remapped_resolver);
    }

    // --proxy-server
    net::DhcpProxyScriptFetcherFactory dhcp_factory;
    if (command_line.HasSwitch(switches::kNoProxyServer)) {
      storage_->set_proxy_service(net::ProxyService::CreateDirect());
    } else if (command_line.HasSwitch(switches::kProxyServer)) {
      net::ProxyConfig proxy_config;
      proxy_config.proxy_rules().ParseFromString(
          command_line.GetSwitchValueASCII(switches::kProxyServer));
      proxy_config.proxy_rules().bypass_rules.ParseFromString(
          command_line.GetSwitchValueASCII(switches::kProxyBypassList));
      storage_->set_proxy_service(net::ProxyService::CreateFixed(proxy_config));
    } else if (command_line.HasSwitch(switches::kProxyPacUrl)) {
      auto proxy_config = net::ProxyConfig::CreateFromCustomPacURL(
          GURL(command_line.GetSwitchValueASCII(switches::kProxyPacUrl)));
      proxy_config.set_pac_mandatory(true);
      storage_->set_proxy_service(net::ProxyService::CreateFixed(
          proxy_config));
    } else {
      storage_->set_proxy_service(
          net::CreateProxyServiceUsingV8ProxyResolver(
              std::move(proxy_config_service_),
              new net::ProxyScriptFetcherImpl(url_request_context_.get()),
              dhcp_factory.Create(url_request_context_.get()),
              host_resolver.get(),
              nullptr,
              url_request_context_->network_delegate()));
    }

    std::vector<std::string> schemes;
    schemes.push_back(std::string("basic"));
    schemes.push_back(std::string("digest"));
    schemes.push_back(std::string("ntlm"));
    schemes.push_back(std::string("negotiate"));
#if defined(OS_POSIX)
    http_auth_preferences_.reset(new net::HttpAuthPreferences(schemes,
                                                              std::string()));
#else
    http_auth_preferences_.reset(new net::HttpAuthPreferences(schemes));
#endif

    // --auth-server-whitelist
    if (command_line.HasSwitch(switches::kAuthServerWhitelist)) {
      http_auth_preferences_->set_server_whitelist(
          command_line.GetSwitchValueASCII(switches::kAuthServerWhitelist));
    }

    // --auth-negotiate-delegate-whitelist
    if (command_line.HasSwitch(switches::kAuthNegotiateDelegateWhitelist)) {
      http_auth_preferences_->set_delegate_whitelist(
          command_line.GetSwitchValueASCII(switches::kAuthNegotiateDelegateWhitelist));
    }

    auto auth_handler_factory =
        net::HttpAuthHandlerRegistryFactory::Create(
            http_auth_preferences_.get(), host_resolver.get());

    storage_->set_cert_verifier(delegate_->CreateCertVerifier());
    storage_->set_transport_security_state(
        base::WrapUnique(new net::TransportSecurityState));
    storage_->set_ssl_config_service(delegate_->CreateSSLConfigService());
    storage_->set_http_auth_handler_factory(std::move(auth_handler_factory));
    std::unique_ptr<net::HttpServerProperties> server_properties(
        new net::HttpServerPropertiesImpl);
    storage_->set_http_server_properties(std::move(server_properties));

    auto multi_log_ct_verifier = new net::MultiLogCTVerifier();
    multi_log_ct_verifier->AddLogs(net::ct::CreateLogVerifiersForKnownLogs());
    cert_transparency_verifier_.reset(multi_log_ct_verifier);
    ct_policy_enforcer_.reset(new net::CTPolicyEnforcer());

    net::HttpNetworkSession::Params network_session_params;
    network_session_params.cert_verifier = url_request_context_->cert_verifier();
    network_session_params.proxy_service = url_request_context_->proxy_service();
    network_session_params.ssl_config_service = url_request_context_->ssl_config_service();
    network_session_params.http_server_properties = url_request_context_->http_server_properties();
    network_session_params.ignore_certificate_errors = false;
    network_session_params.transport_security_state =
        url_request_context_->transport_security_state();
    network_session_params.channel_id_service =
        url_request_context_->channel_id_service();
    network_session_params.http_auth_handler_factory =
        url_request_context_->http_auth_handler_factory();
    network_session_params.net_log = url_request_context_->net_log();
    network_session_params.cert_transparency_verifier =
        cert_transparency_verifier_.get();
    network_session_params.ct_policy_enforcer = ct_policy_enforcer_.get();

    // --disable-http2
    if (command_line.HasSwitch(switches::kDisableHttp2)) {
      network_session_params.enable_spdy31 = false;
      network_session_params.enable_http2 = false;
    }

    // --ignore-certificate-errors
    if (command_line.HasSwitch(switches::kIgnoreCertificateErrors))
      network_session_params.ignore_certificate_errors = true;

    // --host-rules
    if (command_line.HasSwitch(switches::kHostRules)) {
      host_mapping_rules_.reset(new net::HostMappingRules);
      host_mapping_rules_->SetRulesFromString(command_line.GetSwitchValueASCII(switches::kHostRules));
      network_session_params.host_mapping_rules = host_mapping_rules_.get();
    }

    // Give |storage_| ownership at the end in case it's |mapped_host_resolver|.
    storage_->set_host_resolver(std::move(host_resolver));
    network_session_params.host_resolver = url_request_context_->host_resolver();

    http_network_session_.reset(
        new net::HttpNetworkSession(network_session_params));
    std::unique_ptr<net::HttpCache::BackendFactory> backend;
    if (in_memory_) {
      backend = net::HttpCache::DefaultBackend::InMemory(0);
    } else {
      backend.reset(delegate_->CreateHttpCacheBackendFactory(base_path_));
    }

    if (network_controller_handle_) {
      storage_->set_http_transaction_factory(base::WrapUnique(
          new net::HttpCache(
              base::WrapUnique(new DevToolsNetworkTransactionFactory(
                  network_controller_handle_->GetController(), http_network_session_.get())),
              std::move(backend),
              false)));
    } else {
      storage_->set_http_transaction_factory(base::WrapUnique(
          new net::HttpCache(http_network_session_.get(),
                             std::move(backend),
                             false)));
    }

    std::unique_ptr<net::URLRequestJobFactory> job_factory =
        delegate_->CreateURLRequestJobFactory(&protocol_handlers_);
    job_factory_ = job_factory.get();

    // Set up interceptors in the reverse order.
    std::unique_ptr<net::URLRequestJobFactory> top_job_factory =
        std::move(job_factory);
    content::URLRequestInterceptorScopedVector::reverse_iterator it;
    for (it = protocol_interceptors_.rbegin();
         it != protocol_interceptors_.rend();
         ++it) {
      top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
          std::move(top_job_factory), base::WrapUnique(*it)));
    }
    protocol_interceptors_.weak_clear();

    storage_->set_job_factory(std::move(top_job_factory));
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner> URLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO);
}

}  // namespace brightray
