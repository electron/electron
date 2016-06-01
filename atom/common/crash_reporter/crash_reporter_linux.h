// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_LINUX_H_
#define ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_LINUX_H_

#include <string>

#include "atom/common/crash_reporter/crash_reporter.h"
#include "atom/common/crash_reporter/linux/crash_dump_handler.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace google_breakpad {
class ExceptionHandler;
class MinidumpDescriptor;
}

namespace crash_reporter {

class CrashReporterLinux : public CrashReporter {
 public:
  static CrashReporterLinux* GetInstance();

  void InitBreakpad(const std::string& product_name,
                    const std::string& version,
                    const std::string& company_name,
                    const std::string& submit_url,
                    bool auto_submit,
                    bool skip_system_crash_handler) override;
  void SetUploadParameters() override;

 private:
  friend struct base::DefaultSingletonTraits<CrashReporterLinux>;

  CrashReporterLinux();
  virtual ~CrashReporterLinux();

  void EnableCrashDumping(const std::string& product_name);

  static bool CrashDone(const google_breakpad::MinidumpDescriptor& minidump,
                        void* context,
                        const bool succeeded);

  std::unique_ptr<google_breakpad::ExceptionHandler> breakpad_;
  CrashKeyStorage crash_keys_;

  uint64_t process_start_time_;
  pid_t pid_;
  std::string upload_url_;

  DISALLOW_COPY_AND_ASSIGN(CrashReporterLinux);
};
}  // namespace crash_reporter

#endif  // ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_LINUX_H_
