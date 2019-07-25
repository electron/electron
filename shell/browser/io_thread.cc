// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/io_thread.h"

#include <string>
#include <utility>

#include "components/net_log/chrome_net_log.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "net/cert/caching_cert_verifier.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_proc.h"
#include "net/cert/multi_threaded_cert_verifier.h"
#include "net/log/net_log_util.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/network/public/mojom/net_log.mojom.h"
#include "services/network/url_request_context_builder_mojo.h"
#include "shell/browser/net/url_request_context_getter.h"

using content::BrowserThread;

namespace {

// Parses the desired granularity of NetLog capturing specified by the command
// line.
net::NetLogCaptureMode GetNetCaptureModeFromCommandLine(
    const base::CommandLine& command_line) {
  base::StringPiece switch_name = network::switches::kNetLogCaptureMode;

  if (command_line.HasSwitch(switch_name)) {
    std::string value = command_line.GetSwitchValueASCII(switch_name);

    if (value == "Default")
      return net::NetLogCaptureMode::kDefault;
    if (value == "IncludeCookiesAndCredentials")
      return net::NetLogCaptureMode::kIncludeSensitive;
    if (value == "IncludeSocketBytes")
      return net::NetLogCaptureMode::kEverything;

    LOG(ERROR) << "Unrecognized value for --" << switch_name;
  }

  return net::NetLogCaptureMode::kDefault;
}

}  // namespace

IOThread::IOThread(
    SystemNetworkContextManager* system_network_context_manager) {
  BrowserThread::SetIOThreadDelegate(this);

  system_network_context_manager->SetUp(
      &network_context_request_, &network_context_params_,
      &http_auth_static_params_, &http_auth_dynamic_params_);
}

IOThread::~IOThread() {
  BrowserThread::SetIOThreadDelegate(nullptr);
}

void IOThread::RegisterURLRequestContextGetter(
    electron::URLRequestContextGetter* getter) {
  base::AutoLock lock(lock_);

  DCHECK(!base::FeatureList::IsEnabled(network::features::kNetworkService));
  DCHECK_EQ(0u, request_context_getters_.count(getter));
  request_context_getters_.insert(getter);
}

void IOThread::DeregisterURLRequestContextGetter(
    electron::URLRequestContextGetter* getter) {
  base::AutoLock lock(lock_);

  DCHECK(!base::FeatureList::IsEnabled(network::features::kNetworkService));
  DCHECK_EQ(1u, request_context_getters_.count(getter));
  request_context_getters_.erase(getter);
}

void IOThread::Init() {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    std::unique_ptr<network::URLRequestContextBuilderMojo> builder =
        std::make_unique<network::URLRequestContextBuilderMojo>();

    // Enable file:// support.
    builder->set_file_enabled(true);

    auto cert_verifier = std::make_unique<net::CachingCertVerifier>(
        std::make_unique<net::MultiThreadedCertVerifier>(
            net::CertVerifyProc::CreateDefault(nullptr)));
    builder->SetCertVerifier(std::move(cert_verifier));

    // Create the network service, so that shared host resolver
    // gets created which is required to set the auth preferences below.
    network::NetworkService* network_service = content::GetNetworkServiceImpl();
    network_service->SetUpHttpAuth(std::move(http_auth_static_params_));
    network_service->ConfigureHttpAuthPrefs(
        std::move(http_auth_dynamic_params_));

    const base::CommandLine* command_line =
        base::CommandLine::ForCurrentProcess();
    // start net log trace if --log-net-log is passed in the command line.
    if (command_line->HasSwitch(network::switches::kLogNetLog)) {
      base::FilePath log_file =
          command_line->GetSwitchValuePath(network::switches::kLogNetLog);
      base::File file(log_file,
                      base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
      if (log_file.empty() || !file.IsValid()) {
        LOG(ERROR) << "Failed opening NetLog: " << log_file.value();
      } else {
        auto platform_dict = net_log::GetPlatformConstantsForNetLog(
            base::CommandLine::ForCurrentProcess()->GetCommandLineString(),
            std::string(ELECTRON_PRODUCT_NAME));
        network_service->StartNetLog(
            std::move(file), GetNetCaptureModeFromCommandLine(*command_line),
            platform_dict ? std::move(*platform_dict)
                          : base::DictionaryValue());
      }
    }

    system_network_context_ = network_service->CreateNetworkContextWithBuilder(
        std::move(network_context_request_), std::move(network_context_params_),
        std::move(builder), &system_request_context_);
  }
}

void IOThread::CleanUp() {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    system_request_context_->proxy_resolution_service()->OnShutdown();

    base::AutoLock lock(lock_);
    for (auto* getter : request_context_getters_) {
      getter->NotifyContextShuttingDown();
    }

    system_network_context_.reset();
  }
}
