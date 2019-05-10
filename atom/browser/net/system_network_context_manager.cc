// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/system_network_context_manager.h"

#include <string>
#include <utility>

#include "atom/browser/io_thread.h"
#include "atom/common/application_info.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"
#include "components/net_log/net_export_file_writer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_features.h"
#include "content/public/common/service_names.mojom.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "net/net_buildflags.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/cross_thread_shared_url_loader_factory_info.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

base::LazyInstance<SystemNetworkContextManager>::Leaky
    g_system_network_context_manager = LAZY_INSTANCE_INITIALIZER;

namespace {

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

  auth_dynamic_params->server_whitelist =
      command_line->GetSwitchValueASCII(atom::switches::kAuthServerWhitelist);
  auth_dynamic_params->delegate_whitelist = command_line->GetSwitchValueASCII(
      atom::switches::kAuthNegotiateDelegateWhitelist);
  auth_dynamic_params->enable_negotiate_port =
      command_line->HasSwitch(atom::switches::kEnableAuthNegotiatePort);

  return auth_dynamic_params;
}

}  // namespace

// SharedURLLoaderFactory backed by a SystemNetworkContextManager and its
// network context. Transparently handles crashes.
class SystemNetworkContextManager::URLLoaderFactoryForSystem
    : public network::SharedURLLoaderFactory {
 public:
  explicit URLLoaderFactoryForSystem(SystemNetworkContextManager* manager)
      : manager_(manager) {}

  // mojom::URLLoaderFactory implementation:

  void CreateLoaderAndStart(network::mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& url_request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!manager_)
      return;
    manager_->GetURLLoaderFactory()->CreateLoaderAndStart(
        std::move(request), routing_id, request_id, options, url_request,
        std::move(client), traffic_annotation);
  }

  void Clone(network::mojom::URLLoaderFactoryRequest request) override {
    if (!manager_)
      return;
    manager_->GetURLLoaderFactory()->Clone(std::move(request));
  }

  // SharedURLLoaderFactory implementation:
  std::unique_ptr<network::SharedURLLoaderFactoryInfo> Clone() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    return std::make_unique<network::CrossThreadSharedURLLoaderFactoryInfo>(
        this);
  }

  void Shutdown() { manager_ = nullptr; }

 private:
  friend class base::RefCounted<URLLoaderFactoryForSystem>;
  ~URLLoaderFactoryForSystem() override {}

  SystemNetworkContextManager* manager_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactoryForSystem);
};

network::mojom::NetworkContext* SystemNetworkContextManager::GetContext() {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    // SetUp should already have been called.
    DCHECK(io_thread_network_context_);
    return io_thread_network_context_.get();
  }

  if (!network_service_network_context_ ||
      network_service_network_context_.encountered_error()) {
    // This should call into OnNetworkServiceCreated(), which will re-create
    // the network service, if needed. There's a chance that it won't be
    // invoked, if the NetworkContext has encountered an error but the
    // NetworkService has not yet noticed its pipe was closed. In that case,
    // trying to create a new NetworkContext would fail, anyways, and hopefully
    // a new NetworkContext will be created on the next GetContext() call.
    content::GetNetworkService();
    DCHECK(network_service_network_context_);
  }
  return network_service_network_context_.get();
}

network::mojom::URLLoaderFactory*
SystemNetworkContextManager::GetURLLoaderFactory() {
  // Create the URLLoaderFactory as needed.
  if (url_loader_factory_ && !url_loader_factory_.encountered_error()) {
    return url_loader_factory_.get();
  }

  network::mojom::URLLoaderFactoryParamsPtr params =
      network::mojom::URLLoaderFactoryParams::New();
  params->process_id = network::mojom::kBrowserProcessId;
  params->is_corb_enabled = false;
  GetContext()->CreateURLLoaderFactory(mojo::MakeRequest(&url_loader_factory_),
                                       std::move(params));
  return url_loader_factory_.get();
}

scoped_refptr<network::SharedURLLoaderFactory>
SystemNetworkContextManager::GetSharedURLLoaderFactory() {
  return shared_url_loader_factory_;
}

net_log::NetExportFileWriter*
SystemNetworkContextManager::GetNetExportFileWriter() {
  if (!net_export_file_writer_) {
    net_export_file_writer_ = std::make_unique<net_log::NetExportFileWriter>();
  }
  return net_export_file_writer_.get();
}

// static
network::mojom::NetworkContextParamsPtr
SystemNetworkContextManager::CreateDefaultNetworkContextParams() {
  network::mojom::NetworkContextParamsPtr network_context_params =
      network::mojom::NetworkContextParams::New();

  network_context_params->enable_brotli =
      base::FeatureList::IsEnabled(features::kBrotliEncoding);

  network_context_params->enable_referrers = true;

  network_context_params->proxy_resolver_factory =
      ChromeMojoProxyResolverFactory::CreateWithStrongBinding().PassInterface();

  return network_context_params;
}

void SystemNetworkContextManager::SetUp(
    network::mojom::NetworkContextRequest* network_context_request,
    network::mojom::NetworkContextParamsPtr* network_context_params,
    network::mojom::HttpAuthStaticParamsPtr* http_auth_static_params,
    network::mojom::HttpAuthDynamicParamsPtr* http_auth_dynamic_params) {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    *network_context_request = mojo::MakeRequest(&io_thread_network_context_);
    *network_context_params = CreateNetworkContextParams();
  } else {
    // Just use defaults if the network service is enabled, since
    // CreateNetworkContextParams() can only be called once.
    *network_context_params = CreateDefaultNetworkContextParams();
  }
  *http_auth_static_params = CreateHttpAuthStaticParams();
  *http_auth_dynamic_params = CreateHttpAuthDynamicParams();
}

SystemNetworkContextManager::SystemNetworkContextManager()
    : proxy_config_monitor_(g_browser_process->local_state()) {
  shared_url_loader_factory_ = new URLLoaderFactoryForSystem(this);
}

SystemNetworkContextManager::~SystemNetworkContextManager() {
  shared_url_loader_factory_->Shutdown();
}

void SystemNetworkContextManager::OnNetworkServiceCreated(
    network::mojom::NetworkService* network_service) {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;

  network_service->SetUpHttpAuth(CreateHttpAuthStaticParams());
  network_service->ConfigureHttpAuthPrefs(CreateHttpAuthDynamicParams());

  // The system NetworkContext must be created first, since it sets
  // |primary_network_context| to true.
  network_service->CreateNetworkContext(
      MakeRequest(&network_service_network_context_),
      CreateNetworkContextParams());
}

network::mojom::NetworkContextParamsPtr
SystemNetworkContextManager::CreateNetworkContextParams() {
  // TODO(mmenke): Set up parameters here (in memory cookie store, etc).
  network::mojom::NetworkContextParamsPtr network_context_params =
      CreateDefaultNetworkContextParams();

  network_context_params->context_name = std::string("system");

  network_context_params->user_agent = atom::GetApplicationUserAgent();

  network_context_params->http_cache_enabled = false;

  // These are needed for PAC scripts that use file or data URLs (Or FTP URLs?).
  // TODO(crbug.com/839566): remove file support for all cases.
  network_context_params->enable_data_url_support = true;
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService))
    network_context_params->enable_file_url_support = true;
#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  network_context_params->enable_ftp_url_support = true;
#endif

  network_context_params->primary_network_context = true;

  proxy_config_monitor_.AddToNetworkContextParams(network_context_params.get());

  return network_context_params;
}
