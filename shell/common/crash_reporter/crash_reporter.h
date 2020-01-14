// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_H_
#define SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"

namespace gin_helper {
class Dictionary;
}

namespace crash_reporter {

extern const char kCrashpadProcess[];
extern const char kCrashesDirectoryKey[];

class CrashReporter {
 public:
  typedef std::map<std::string, std::string> StringMap;
  typedef std::pair<int, std::string> UploadReportResult;  // upload-date, id

  static CrashReporter* GetInstance();
  // FIXME(zcbenz): We should not do V8 in this file, this method should only
  // accept C++ struct as parameter, and atom_api_crash_reporter.cc is
  // responsible for parsing the parameter from JavaScript.
  static void StartInstance(const gin_helper::Dictionary& options);

  bool IsInitialized();
  void Start(const std::string& product_name,
             const std::string& company_name,
             const std::string& submit_url,
             const base::FilePath& crashes_dir,
             bool upload_to_server,
             bool skip_system_crash_handler,
             const StringMap& extra_parameters);

  virtual std::vector<CrashReporter::UploadReportResult> GetUploadedReports(
      const base::FilePath& crashes_dir);

  virtual void SetUploadToServer(bool upload_to_server);
  virtual bool GetUploadToServer();
  virtual void AddExtraParameter(const std::string& key,
                                 const std::string& value);
  virtual void RemoveExtraParameter(const std::string& key);
  virtual std::map<std::string, std::string> GetParameters() const;

 protected:
  CrashReporter();
  virtual ~CrashReporter();

  virtual void Init(const std::string& product_name,
                    const std::string& company_name,
                    const std::string& submit_url,
                    const base::FilePath& crashes_dir,
                    bool upload_to_server,
                    bool skip_system_crash_handler);
  virtual void SetUploadParameters();

  StringMap upload_parameters_;
  std::string process_type_;

 private:
  bool is_initialized_ = false;
  void SetUploadParameters(const StringMap& parameters);

  DISALLOW_COPY_AND_ASSIGN(CrashReporter);
};

}  // namespace crash_reporter

#endif  // SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_H_
