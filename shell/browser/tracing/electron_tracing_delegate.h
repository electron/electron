// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_TRACING_ELECTRON_TRACING_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_TRACING_ELECTRON_TRACING_DELEGATE_H_

#include "components/version_info/channel.h"
#include "content/public/browser/tracing_delegate.h"
#include "services/tracing/public/cpp/perfetto/metadata_data_source.h"

namespace electron {

class ElectronTracingDelegate : public content::TracingDelegate {
 public:
  ElectronTracingDelegate();
  ~ElectronTracingDelegate() override;

  // disable copy
  ElectronTracingDelegate(const ElectronTracingDelegate&) = delete;
  ElectronTracingDelegate& operator=(const ElectronTracingDelegate&) = delete;

  // content::TracingDelegate:
  std::string RecordSerializedSystemProfileMetrics() const override;
  tracing::MetadataDataSource::BundleRecorder
  CreateSystemProfileMetadataRecorder() const override;
  tracing::MetadataDataSource::ChromeMetadataRecorder
  CreateChromeMetadataPacketRecorder() const override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_TRACING_ELECTRON_TRACING_DELEGATE_H_
