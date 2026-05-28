// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_TRACING_ELECTRON_BACKGROUND_TRACING_METRICS_PROVIDER_H_
#define ELECTRON_SHELL_BROWSER_TRACING_ELECTRON_BACKGROUND_TRACING_METRICS_PROVIDER_H_

#include "components/tracing/common/background_tracing_metrics_provider.h"

namespace electron {

class ElectronBackgroundTracingMetricsProvider
    : public tracing::BackgroundTracingMetricsProvider {
 public:
  ElectronBackgroundTracingMetricsProvider();
  ~ElectronBackgroundTracingMetricsProvider() override;

  // disable copy
  ElectronBackgroundTracingMetricsProvider(
      const ElectronBackgroundTracingMetricsProvider&) = delete;
  ElectronBackgroundTracingMetricsProvider& operator=(
      const ElectronBackgroundTracingMetricsProvider&) = delete;

  // metrics::MetricsProvider:
  void RecordCoreSystemProfileMetrics(
      metrics::SystemProfileProto& system_profile_proto) override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_TRACING_ELECTRON_BACKGROUND_TRACING_METRICS_PROVIDER_H_
