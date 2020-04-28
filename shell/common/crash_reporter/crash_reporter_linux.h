// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_LINUX_H_
#define SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_LINUX_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "shell/common/crash_reporter/crash_reporter.h"
#include "shell/common/crash_reporter/linux/crash_dump_handler.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace google_breakpad {
class ExceptionHandler;
class MinidumpDescriptor;
}  // namespace google_breakpad

namespace crash_reporter {

class CrashReporterLinux : public CrashReporter {
 public:
  static CrashReporterLinux* GetInstance();

  void Init(const std::string& submit_url,
            const base::FilePath& crashes_dir,
            bool upload_to_server,
            bool skip_system_crash_handler,
            bool rate_limit,
            bool compress,
            const StringMap& global_extra) override;
  void InitInChild() override;
  void SetUploadToServer(bool upload_to_server) override;
  void SetUploadParameters() override;
  bool GetUploadToServer() override;
  void AddExtraParameter(const std::string& key,
                         const std::string& value) override;
  void RemoveExtraParameter(const std::string& key) override;

  base::FilePath GetCrashesDirectory() override;

 private:
  friend struct base::DefaultSingletonTraits<CrashReporterLinux>;

  CrashReporterLinux();
  ~CrashReporterLinux() override;

  void EnableCrashDumping(const base::FilePath& crashes_dir);
  void EnableNonBrowserCrashDumping();

  static bool CrashDone(const google_breakpad::MinidumpDescriptor& minidump,
                        void* context,
                        const bool succeeded);

  std::unique_ptr<google_breakpad::ExceptionHandler> breakpad_;
  using CrashKeyStorage = crash_reporter::internal::TransitionalCrashKeyStorage;
  std::unique_ptr<CrashKeyStorage> crash_keys_;

  pid_t pid_ = 0;
  bool upload_to_server_ = true;

  base::FilePath crashes_dir_;

  DISALLOW_COPY_AND_ASSIGN(CrashReporterLinux);
};
}  // namespace crash_reporter

#endif  // SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_LINUX_H_
