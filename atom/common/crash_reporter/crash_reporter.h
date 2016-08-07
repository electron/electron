// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"

namespace crash_reporter {

class CrashReporter {
 public:
  typedef std::map<std::string, std::string> StringMap;
  typedef std::pair<int, std::string> UploadReportResult;  // upload-date, id

  static CrashReporter* GetInstance();

  void Start(const std::string& product_name,
             const std::string& company_name,
             const std::string& submit_url,
             bool auto_submit,
             bool skip_system_crash_handler,
             const StringMap& extra_parameters);

  virtual std::vector<CrashReporter::UploadReportResult> GetUploadedReports(
      const std::string& path);

 protected:
  CrashReporter();
  virtual ~CrashReporter();

  virtual void InitBreakpad(const std::string& product_name,
                            const std::string& version,
                            const std::string& company_name,
                            const std::string& submit_url,
                            bool auto_submit,
                            bool skip_system_crash_handler);
  virtual void SetUploadParameters();

  StringMap upload_parameters_;
  bool is_browser_;

 private:
  void SetUploadParameters(const StringMap& parameters);

  DISALLOW_COPY_AND_ASSIGN(CrashReporter);
};

}  // namespace crash_reporter
