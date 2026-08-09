// Copyright (c) 2026 Niklas Wenzel
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_METRICS_ELECTRON_METRICS_LOG_UPLOADER_H_
#define ELECTRON_SHELL_BROWSER_METRICS_ELECTRON_METRICS_LOG_UPLOADER_H_

#include <string>

#include "components/metrics/metrics_log_uploader.h"

namespace electron {

class ElectronMetricsLogUploader : public metrics::MetricsLogUploader {
 public:
  explicit ElectronMetricsLogUploader(
      const metrics::MetricsLogUploader::UploadCallback& on_upload_complete);

  ~ElectronMetricsLogUploader() override;

  // disable copy
  ElectronMetricsLogUploader(const ElectronMetricsLogUploader&) = delete;
  ElectronMetricsLogUploader& operator=(const ElectronMetricsLogUploader&) =
      delete;

  void UploadLog(const std::string& compressed_log_data,
                 const metrics::LogMetadata& log_metadata,
                 const std::string& log_hash,
                 const std::string& log_signature,
                 const metrics::ReportingInfo& reporting_info) override;

 private:
  const metrics::MetricsLogUploader::UploadCallback on_upload_complete_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_METRICS_ELECTRON_METRICS_LOG_UPLOADER_H_
