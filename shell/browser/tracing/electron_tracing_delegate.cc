// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/tracing/electron_tracing_delegate.h"

#include <string>

#include "base/functional/bind.h"
#include "components/tracing/common/background_tracing_metrics_provider.h"
#include "components/tracing/common/system_profile_metadata_recorder.h"
#include "third_party/metrics_proto/system_profile.pb.h"

namespace electron {

ElectronTracingDelegate::ElectronTracingDelegate() = default;

ElectronTracingDelegate::~ElectronTracingDelegate() = default;

// Mirrors |ChromeTracingDelegate::RecordSerializedSystemProfileMetrics|.
std::string ElectronTracingDelegate::RecordSerializedSystemProfileMetrics()
    const {
  metrics::SystemProfileProto system_profile_proto;
  auto recorder = tracing::BackgroundTracingMetricsProvider::
      GetSystemProfileMetricsRecorder();
  if (!recorder) {
    return std::string();
  }
  recorder.Run(system_profile_proto);
  std::string serialized_system_profile;
  system_profile_proto.SerializeToString(&serialized_system_profile);
  return serialized_system_profile;
}

tracing::MetadataDataSource::BundleRecorder
ElectronTracingDelegate::CreateSystemProfileMetadataRecorder() const {
  return base::BindRepeating(&tracing::RecordSystemProfileMetadata);
}

}  // namespace electron
