// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/metrics/electron_metrics_log_uploader.h"

#include "net/base/net_errors.h"

namespace electron {

ElectronMetricsLogUploader::ElectronMetricsLogUploader(
    const metrics::MetricsLogUploader::UploadCallback& on_upload_complete)
    : on_upload_complete_(on_upload_complete) {}

ElectronMetricsLogUploader::~ElectronMetricsLogUploader() = default;

void ElectronMetricsLogUploader::UploadLog(const std::string&,
                                           const metrics::LogMetadata&,
                                           const std::string&,
                                           const std::string&,
                                           const metrics::ReportingInfo&) {
  on_upload_complete_.Run(0, net::ERR_NOT_IMPLEMENTED, false, true,
                          "Not implemented");
}

}  // namespace electron
