// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_MAC_H_
#define ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_MAC_H_

#include <string>
#include <vector>

#include "atom/common/crash_reporter/crash_reporter.h"
#include "base/compiler_specific.h"
#include "base/strings/string_piece.h"
#include "vendor/crashpad/client/crash_report_database.h"
#include "vendor/crashpad/client/simple_string_dictionary.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace crash_reporter {

class CrashReporterMac : public CrashReporter {
 public:
  static CrashReporterMac* GetInstance();

  void InitBreakpad(const std::string& product_name,
                    const std::string& version,
                    const std::string& company_name,
                    const std::string& submit_url,
                    const base::FilePath& crashes_dir,
                    bool upload_to_server,
                    bool skip_system_crash_handler) override;
  void SetUploadParameters() override;
  void SetUploadToServer(bool upload_to_server) override;
  bool GetUploadToServer() override;

 private:
  friend struct base::DefaultSingletonTraits<CrashReporterMac>;

  CrashReporterMac();
  virtual ~CrashReporterMac();

  void SetUploadsEnabled(bool enable_uploads);
  void SetCrashKeyValue(const base::StringPiece& key,
                        const base::StringPiece& value);

  std::vector<UploadReportResult> GetUploadedReports(
      const base::FilePath& crashes_dir) override;

  std::unique_ptr<crashpad::SimpleStringDictionary> simple_string_dictionary_;
  std::unique_ptr<crashpad::CrashReportDatabase> database_;

  DISALLOW_COPY_AND_ASSIGN(CrashReporterMac);
};

}  // namespace crash_reporter

#endif  // ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_MAC_H_
