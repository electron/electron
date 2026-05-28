// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/metrics/electron_metrics_service_client.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/version_info/channel.h"
#include "chrome/browser/browser_process.h"
#include "components/metrics/metrics_service.h"
#include "components/metrics/metrics_state_manager.h"
#include "components/metrics/version_utils.h"
#include "shell/browser/metrics/electron_metrics_log_uploader.h"
#include "shell/browser/tracing/electron_background_tracing_metrics_provider.h"
#include "third_party/metrics_proto/chrome_user_metrics_extension.pb.h"

namespace electron {

ElectronMetricsServiceClient::ElectronMetricsServiceClient()
    : metrics_state_manager_(metrics::MetricsStateManager::Create(
          g_browser_process->local_state(),
          this,
          std::wstring(),
          base::FilePath(),
          metrics::StartupVisibility::kUnknown)),
      metrics_service_(std::make_unique<metrics::MetricsService>(
          metrics_state_manager_.get(),
          this,
          g_browser_process->local_state())) {
  RegisterMetricsServiceProviders();
}

ElectronMetricsServiceClient::~ElectronMetricsServiceClient() = default;

// static
void ElectronMetricsServiceClient::RegisterMetricsPrefs(
    PrefRegistrySimple* registry) {
  metrics::MetricsService::RegisterPrefs(registry);
}

variations::SyntheticTrialRegistry*
ElectronMetricsServiceClient::GetSyntheticTrialRegistry() {
  return nullptr;
}

metrics::MetricsService* ElectronMetricsServiceClient::GetMetricsService() {
  return metrics_service_.get();
}

void ElectronMetricsServiceClient::SetMetricsClientId(
    const std::string& client_id) {}

int32_t ElectronMetricsServiceClient::GetProduct() {
  return metrics::ChromeUserMetricsExtension::CHROME;
}

std::string ElectronMetricsServiceClient::GetApplicationLocale() {
  return g_browser_process ? g_browser_process->GetApplicationLocale()
                           : std::string();
}

const network_time::NetworkTimeTracker*
ElectronMetricsServiceClient::GetNetworkTimeTracker() {
  return g_browser_process ? g_browser_process->network_time_tracker()
                           : nullptr;
}

bool ElectronMetricsServiceClient::GetBrand(std::string* brand_code) {
  return false;
}

metrics::SystemProfileProto::Channel
ElectronMetricsServiceClient::GetChannel() {
  return metrics::AsProtobufChannel(version_info::Channel::UNKNOWN);
}

bool ElectronMetricsServiceClient::IsExtendedStableChannel() {
  return false;
}

std::string ElectronMetricsServiceClient::GetVersionString() {
  return metrics::GetVersionString();
}

void ElectronMetricsServiceClient::CollectFinalMetricsForLog(
    base::OnceClosure done_callback) {
  if (done_callback)
    std::move(done_callback).Run();
}

std::unique_ptr<metrics::MetricsLogUploader>
ElectronMetricsServiceClient::CreateUploader(
    const GURL& server_url,
    const GURL& insecure_server_url,
    std::string_view mime_type,
    metrics::MetricsLogUploader::MetricServiceType service_type,
    const metrics::MetricsLogUploader::UploadCallback& on_upload_complete) {
  return std::make_unique<ElectronMetricsLogUploader>(on_upload_complete);
}

base::TimeDelta ElectronMetricsServiceClient::GetStandardUploadInterval() {
  return base::TimeDelta::FiniteMax();
}

bool ElectronMetricsServiceClient::IsConsentGiven() const {
  return false;
}

void ElectronMetricsServiceClient::RegisterMetricsServiceProviders() {
  metrics_service_->RegisterMetricsProvider(
      std::make_unique<ElectronBackgroundTracingMetricsProvider>());
}

}  // namespace electron
