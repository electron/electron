// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_MAC_H_
#define SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_MAC_H_

#include <memory>
#include <string>

#include "shell/common/crash_reporter/crash_reporter_crashpad.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace crash_reporter {

class CrashReporterMac : public CrashReporterCrashpad {
 public:
  static CrashReporterMac* GetInstance();

  void Init(const std::string& submit_url,
            const base::FilePath& crashes_dir,
            bool upload_to_server,
            bool skip_system_crash_handler,
            bool rate_limit,
            bool compress) override;
  void SetUploadParameters() override;

 private:
  friend struct base::DefaultSingletonTraits<CrashReporterMac>;

  CrashReporterMac();
  ~CrashReporterMac() override;

  DISALLOW_COPY_AND_ASSIGN(CrashReporterMac);
};

}  // namespace crash_reporter

#endif  // SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_MAC_H_
