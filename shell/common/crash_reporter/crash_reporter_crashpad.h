// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_CRASHPAD_H_
#define SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_CRASHPAD_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/strings/string_piece.h"
#include "shell/common/crash_reporter/crash_reporter.h"
#include "third_party/crashpad/crashpad/client/crash_report_database.h"
#include "third_party/crashpad/crashpad/client/simple_string_dictionary.h"

namespace crash_reporter {

class CrashReporterCrashpad : public CrashReporter {
 public:
  void SetUploadToServer(bool upload_to_server) override;
  bool GetUploadToServer() override;
  void AddExtraParameter(const std::string& key,
                         const std::string& value) override;
  void RemoveExtraParameter(const std::string& key) override;
  std::map<std::string, std::string> GetParameters() const override;

 protected:
  CrashReporterCrashpad();
  ~CrashReporterCrashpad() override;

  void SetUploadsEnabled(bool enable_uploads);
  void SetCrashKeyValue(base::StringPiece key, base::StringPiece value);
  void SetInitialCrashKeyValues();

  std::vector<UploadReportResult> GetUploadedReports(
      const base::FilePath& crashes_dir) override;

  std::unique_ptr<crashpad::SimpleStringDictionary> simple_string_dictionary_;
  std::unique_ptr<crashpad::CrashReportDatabase> database_;

  DISALLOW_COPY_AND_ASSIGN(CrashReporterCrashpad);
};

}  // namespace crash_reporter

#endif  // SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_CRASHPAD_H_
