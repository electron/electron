// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/url_request_context_getter.h"

#include <algorithm>

#include "browser/network_delegate.h"

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/common/content_switches.h"
#include "net/base/host_mapping_rules.h"
#include "net/cert/cert_verifier.h"
#include "net/cookies/cookie_monster.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_cache.h"
#include "net/http/http_server_properties_impl.h"
#include "net/proxy/dhcp_proxy_script_fetcher_factory.h"
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
#include "url/url_constants.h"
#include "storage/browser/quota/special_storage_policy.h"

using content::BrowserThread;

namespace brightray {

namespace {

// Comma-separated list of rules that control how hostnames are mapped.
//
// For example:
//    "MAP * 127.0.0.1" --> Forces all hostnames to be mapped to 127.0.0.1
//    "MAP *.google.com proxy" --> Forces all google.com subdomains to be
//                                 resolved to "proxy".
//    "MAP test.com [::1]:77 --> Forces "test.com" to resolve to IPv6 loopback.
//                               Will also force the port of the resulting
//                               socket address to be 77.
//    "MAP * baz, EXCLUDE www.google.com" --> Remaps everything to "baz",
//                                            except for "www.google.com".
//
// These mappings apply to the endpoint host in a net::URLRequest (the TCP
// connect and host resolver in a direct connection, and the CONNECT in an http
// proxy connection, and the endpoint host in a SOCKS proxy connection).
const char kHostRules[] = "host-rules";

// Don't use a proxy server, always make direct connections. Overrides any
// other proxy server flags that are passed.
const char kNoProxyServer[] = "no-proxy-server";

// Uses a specified proxy server, overrides system settings. This switch only
// affects HTTP and HTTPS requests.
const char kProxyServer[] = "proxy-server";

}  // namespace

net::URLRequestJobFactory* URLRequestContextGetter::Delegate::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector* protocol_interceptors) {
  scoped_ptr<net::URLRequestJobFactoryImpl> job_factory(new net::URLRequestJobFactoryImpl);

  for (auto it = protocol_handlers->begin(); it != protocol_handlers->end(); ++it)
    job_factory->SetProtocolHandler(it->first, it->second.release());
  protocol_handlers->clear();

  job_factory->SetProtocolHandler(url::kDataScheme, new net::DataProtocolHandler);
  job_factory->SetProtocolHandler(url::kFileScheme, new net::FileProtocolHandler(
      BrowserThread::GetBlockingPool()->GetTaskRunnerWithShutdownBehavior(
          base::SequencedWorkerPool::SKIP_ON_SHUTDOWN)));

  // Set up interceptors in the reverse order.
  scoped_ptr<net::URLRequestJobFactory> top_job_factory =
      job_factory.PassAs<net::URLRequestJobFactory>();
  content::URLRequestInterceptorScopedVector::reverse_iterator i;
  for (i = protocol_interceptors->rbegin(); i != protocol_interceptors->rend(); ++i)
    top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
        top_job_factory.Pass(), make_scoped_ptr(*i)));
  protocol_interceptors->weak_clear();

  return top_job_factory.release();
}

URLRequestContextGetter::URLRequestContextGetter(
    Delegate* delegate,
    const base::FilePath& base_path,
    base::MessageLoop* io_loop,
    base::MessageLoop* file_loop,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors)
    : delegate_(delegate),
      base_path_(base_path),
      io_loop_(io_loop),
      file_loop_(file_loop),
      protocol_interceptors_(protocol_interceptors.Pass()) {
  // Must first be created on the UI thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  std::swap(protocol_handlers_, *protocol_handlers);

  // We must create the proxy config service on the UI loop on Linux because it
  // must synchronously run on the glib message loop. This will be passed to
  // the URLRequestContextStorage on the IO thread in GetURLRequestContext().
  proxy_config_service_.reset(net::ProxyService::CreateSystemProxyConfigService(
      io_loop_->message_loop_proxy(), file_loop_->message_loop_proxy()));
}

URLRequestContextGetter::~URLRequestContextGetter() {
}

net::HostResolver* URLRequestContextGetter::host_resolver() {
  return url_request_context_->host_resolver();
}

net::URLRequestContext* URLRequestContextGetter::GetURLRequestContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (!url_request_context_.get()) {
    url_request_context_.reset(new net::URLRequestContext);
    network_delegate_.reset(delegate_->CreateNetworkDelegate());
    url_request_context_->set_network_delegate(network_delegate_.get());

    storage_.reset(new net::URLRequestContextStorage(url_request_context_.get()));
    auto cookie_config = content::CookieStoreConfig(
        base_path_.Append(FILE_PATH_LITERAL("Cookies")),
        content::CookieStoreConfig::EPHEMERAL_SESSION_COOKIES,
        NULL, NULL);
    storage_->set_cookie_store(content::CreateCookieStore(cookie_config));
    storage_->set_channel_id_service(new net::ChannelIDService(
        new net::DefaultChannelIDStore(NULL),
        base::WorkerPool::GetTaskRunner(true)));
    storage_->set_http_user_agent_settings(new net::StaticHttpUserAgentSettings(
        "en-us,en", base::EmptyString()));

    scoped_ptr<net::HostResolver> host_resolver(net::HostResolver::CreateDefaultResolver(NULL));

    // --host-resolver-rules
    if (command_line.HasSwitch(switches::kHostResolverRules)) {
      scoped_ptr<net::MappedHostResolver> remapped_resolver(
          new net::MappedHostResolver(host_resolver.Pass()));
      remapped_resolver->SetRulesFromString(
          command_line.GetSwitchValueASCII(switches::kHostResolverRules));
      host_resolver = remapped_resolver.PassAs<net::HostResolver>();
    }

    // --proxy-server
    net::DhcpProxyScriptFetcherFactory dhcp_factory;
    if (command_line.HasSwitch(kNoProxyServer))
      storage_->set_proxy_service(net::ProxyService::CreateDirect());
    else if (command_line.HasSwitch(kProxyServer))
      storage_->set_proxy_service(net::ProxyService::CreateFixed(
          command_line.GetSwitchValueASCII(kProxyServer)));
    else
      storage_->set_proxy_service(
          net::CreateProxyServiceUsingV8ProxyResolver(
              proxy_config_service_.release(),
              new net::ProxyScriptFetcherImpl(url_request_context_.get()),
              dhcp_factory.Create(url_request_context_.get()),
              host_resolver.get(),
              NULL,
              url_request_context_->network_delegate()));

    storage_->set_cert_verifier(net::CertVerifier::CreateDefault());
    storage_->set_transport_security_state(new net::TransportSecurityState);
    storage_->set_ssl_config_service(new net::SSLConfigServiceDefaults);
    storage_->set_http_auth_handler_factory(
        net::HttpAuthHandlerFactory::CreateDefault(host_resolver.get()));
    scoped_ptr<net::HttpServerProperties> server_properties(
        new net::HttpServerPropertiesImpl);
    storage_->set_http_server_properties(server_properties.Pass());

    net::HttpNetworkSession::Params network_session_params;
    network_session_params.cert_verifier = url_request_context_->cert_verifier();
    network_session_params.proxy_service = url_request_context_->proxy_service();
    network_session_params.ssl_config_service = url_request_context_->ssl_config_service();
    network_session_params.network_delegate = url_request_context_->network_delegate();
    network_session_params.http_server_properties = url_request_context_->http_server_properties();
    network_session_params.ignore_certificate_errors = false;
    network_session_params.transport_security_state =
        url_request_context_->transport_security_state();
    network_session_params.channel_id_service =
        url_request_context_->channel_id_service();
    network_session_params.http_auth_handler_factory =
        url_request_context_->http_auth_handler_factory();

    // --host-rules
    if (command_line.HasSwitch(kHostRules)) {
      host_mapping_rules_.reset(new net::HostMappingRules);
      host_mapping_rules_->SetRulesFromString(command_line.GetSwitchValueASCII(kHostRules));
      network_session_params.host_mapping_rules = host_mapping_rules_.get();
    }

    // Give |storage_| ownership at the end in case it's |mapped_host_resolver|.
    storage_->set_host_resolver(host_resolver.Pass());
    network_session_params.host_resolver = url_request_context_->host_resolver();

    base::FilePath cache_path = base_path_.Append(FILE_PATH_LITERAL("Cache"));
    net::HttpCache::DefaultBackend* backend =
        new net::HttpCache::DefaultBackend(
            net::DISK_CACHE,
            net::CACHE_BACKEND_DEFAULT,
            cache_path,
            0,
            BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE));
    storage_->set_http_transaction_factory(new net::HttpCache(network_session_params, backend));

    storage_->set_job_factory(delegate_->CreateURLRequestJobFactory(
        &protocol_handlers_, &protocol_interceptors_));
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner> URLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO);
}

}  // namespace brightray
