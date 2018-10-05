// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/io_thread.h"
#include "atom/common/options_switches.h"

#include "components/net_log/chrome_net_log.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/network_service.h"

#if defined(USE_NSS_CERTS)
#include "net/cert_net/nss_ocsp.h"
#endif

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include "net/cert/cert_net_fetcher.h"
#include "net/cert_net/cert_net_fetcher_impl.h"
#endif

using content::BrowserThread;

namespace atom {

namespace {

network::mojom::HttpAuthStaticParamsPtr CreateHttpAuthStaticParams() {
  network::mojom::HttpAuthStaticParamsPtr auth_static_params =
      network::mojom::HttpAuthStaticParams::New();

  auth_static_params->supported_schemes = {"basic", "digest", "ntlm",
                                           "negotiate"};

  return auth_static_params;
}

network::mojom::HttpAuthDynamicParamsPtr CreateHttpAuthDynamicParams(
    const base::CommandLine& command_line) {
  network::mojom::HttpAuthDynamicParamsPtr auth_dynamic_params =
      network::mojom::HttpAuthDynamicParams::New();

  auth_dynamic_params->server_whitelist =
      command_line.GetSwitchValueASCII(switches::kAuthServerWhitelist);
  auth_dynamic_params->delegate_whitelist = command_line.GetSwitchValueASCII(
      switches::kAuthNegotiateDelegateWhitelist);

  return auth_dynamic_params;
}

}  // namespace

IOThread::IOThread(net_log::ChromeNetLog* net_log) : net_log_(net_log) {
  BrowserThread::SetIOThreadDelegate(this);
}

IOThread::~IOThread() {
  BrowserThread::SetIOThreadDelegate(nullptr);
}

void IOThread::Init() {
  // Create the network service, so that shared host resolver
  // gets created which is required to set the auth preferences below.
  auto& command_line = *base::CommandLine::ForCurrentProcess();
  auto* network_service = content::GetNetworkServiceImpl();
  network_service->SetUpHttpAuth(CreateHttpAuthStaticParams());
  network_service->ConfigureHttpAuthPrefs(
      CreateHttpAuthDynamicParams(command_line));

  net::URLRequestContextBuilder builder;
  // TODO(deepak1556): We need to respoect user proxy configurations,
  // the following initialization has to happen before any request
  // contexts are utilized by the io thread, so that proper cert validation
  // take place, solutions:
  // 1) Use the request context from default partition, but since
  //    an app can completely run on a custom session without ever creating
  //    the default session, we will have to force create the default session
  //    in those scenarios.
  // 2) Add a new api on app module that sets the proxy configuration
  //    for the global requests, like the cert fetchers below and
  //    geolocation requests.
  // 3) There is also ongoing work in upstream which will eventually allow
  //    localizing these global fetchers to their own URLRequestContexts.
  builder.set_proxy_resolution_service(
      net::ProxyResolutionService::CreateDirect());
  url_request_context_ = builder.Build();
  url_request_context_getter_ = new net::TrivialURLRequestContextGetter(
      url_request_context_.get(), base::ThreadTaskRunnerHandle::Get());

#if defined(USE_NSS_CERTS)
  net::SetURLRequestContextForNSSHttpIO(url_request_context_.get());
#endif
#if defined(OS_LINUX) || defined(OS_MACOSX)
  net::SetGlobalCertNetFetcher(
      net::CreateCertNetFetcher(url_request_context_.get()));
#endif
}

void IOThread::CleanUp() {
#if defined(USE_NSS_CERTS)
  net::SetURLRequestContextForNSSHttpIO(nullptr);
#endif
#if defined(OS_LINUX) || defined(OS_MACOSX)
  net::ShutdownGlobalCertNetFetcher();
#endif
  // Explicitly release before the IO thread gets destroyed.
  url_request_context_.reset();
  url_request_context_getter_ = nullptr;

  if (net_log_)
    net_log_->ShutDownBeforeTaskScheduler();
}

}  // namespace atom
