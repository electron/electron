// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_METRICS_ELECTRON_METRICS_SERVICE_CLIENT_H_
#define ELECTRON_SHELL_BROWSER_METRICS_ELECTRON_METRICS_SERVICE_CLIENT_H_

#include <memory>

#include "components/metrics/enabled_state_provider.h"
#include "components/metrics/metrics_service_client.h"

namespace metrics {
class MetricsService;
class MetricsStateManager;
}  // namespace metrics

namespace variations {
class SyntheticTrialRegistry;
}

namespace electron {

class ElectronMetricsServiceClient : public metrics::MetricsServiceClient,
                                     public metrics::EnabledStateProvider {
 public:
  ElectronMetricsServiceClient();
  ~ElectronMetricsServiceClient() override;

  // disable copy
  ElectronMetricsServiceClient(const ElectronMetricsServiceClient&) = delete;
  ElectronMetricsServiceClient& operator=(const ElectronMetricsServiceClient&) =
      delete;

  static void RegisterMetricsPrefs(PrefRegistrySimple* registry);

  // metrics::MetricsServiceClient:
  variations::SyntheticTrialRegistry* GetSyntheticTrialRegistry() override;
  metrics::MetricsService* GetMetricsService() override;
  void SetMetricsClientId(const std::string& client_id) override;
  int32_t GetProduct() override;
  std::string GetApplicationLocale() override;
  const network_time::NetworkTimeTracker* GetNetworkTimeTracker() override;
  bool GetBrand(std::string* brand_code) override;
  metrics::SystemProfileProto::Channel GetChannel() override;
  bool IsExtendedStableChannel() override;
  std::string GetVersionString() override;
  void CollectFinalMetricsForLog(base::OnceClosure done_callback) override;
  std::unique_ptr<metrics::MetricsLogUploader> CreateUploader(
      const GURL& server_url,
      const GURL& insecure_server_url,
      std::string_view mime_type,
      metrics::MetricsLogUploader::MetricServiceType service_type,
      const metrics::MetricsLogUploader::UploadCallback& on_upload_complete)
      override;
  base::TimeDelta GetStandardUploadInterval() override;

  // metrics::EnabledStateProvider:
  bool IsConsentGiven() const override;

 private:
  void RegisterMetricsServiceProviders();

  std::unique_ptr<metrics::MetricsStateManager> metrics_state_manager_;
  std::unique_ptr<metrics::MetricsService> metrics_service_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_METRICS_ELECTRON_METRICS_SERVICE_CLIENT_H_
