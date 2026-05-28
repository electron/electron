// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/tracing/electron_background_tracing_metrics_provider.h"

#include <memory>
#include <string>

#include "base/version_info/channel.h"
#include "components/metrics/metrics_log.h"
#include "components/metrics/version_utils.h"
#include "shell/browser/electron_browser_client.h"

namespace electron {

ElectronBackgroundTracingMetricsProvider::
    ElectronBackgroundTracingMetricsProvider() = default;

ElectronBackgroundTracingMetricsProvider::
    ~ElectronBackgroundTracingMetricsProvider() = default;

void ElectronBackgroundTracingMetricsProvider::RecordCoreSystemProfileMetrics(
    metrics::SystemProfileProto& system_profile_proto) {
  auto* browser_client = ElectronBrowserClient::Get();
  const std::string application_locale =
      browser_client ? browser_client->GetApplicationLocale() : std::string();

  metrics::MetricsLog::RecordCoreSystemProfile(
      metrics::GetVersionString(),
      metrics::AsProtobufChannel(version_info::Channel::UNKNOWN), false,
      application_locale, std::string(), &system_profile_proto);
}

}  // namespace electron
