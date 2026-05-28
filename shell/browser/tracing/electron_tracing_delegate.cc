// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/tracing/electron_tracing_delegate.h"

#include <string>

#include "base/functional/bind.h"
#include "base/system/sys_info.h"
#include "components/metrics/version_utils.h"
#include "components/tracing/common/background_tracing_metrics_provider.h"
#include "components/tracing/common/system_profile_metadata_recorder.h"
#include "components/version_info/channel.h"
#include "services/tracing/public/cpp/perfetto/metadata_data_source.h"
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

namespace {

// https://chromium-review.googlesource.com/c/chromium/src/+/7770189
// product-version, os-name, and channel from the ChromeEventBundle metadata
// path to typed ChromeMetadataPacket proto fields. Re-add these fields since
// third_party/catapult/tracing/bin/symbolize_trace needs them.
void RecordSystemProfileMetadataWithProductVersion(
    perfetto::protos::pbzero::ChromeEventBundle* bundle) {
  tracing::RecordSystemProfileMetadata(bundle);
  tracing::MetadataDataSource::AddMetadataToBundle(
      "product-version", metrics::GetVersionString(), bundle);
  tracing::MetadataDataSource::AddMetadataToBundle(
      "os-name", base::SysInfo::OperatingSystemName(), bundle);
}

}  // namespace

tracing::MetadataDataSource::BundleRecorder
ElectronTracingDelegate::CreateSystemProfileMetadataRecorder() const {
  return base::BindRepeating(&RecordSystemProfileMetadataWithProductVersion);
}

tracing::MetadataDataSource::ChromeMetadataRecorder
ElectronTracingDelegate::CreateChromeMetadataPacketRecorder() const {
  return base::BindRepeating(&tracing::FillChromeMetadataPacket,
                             version_info::Channel::UNKNOWN);
}

}  // namespace electron
