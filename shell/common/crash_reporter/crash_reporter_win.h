// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_WIN_H_
#define SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_WIN_H_

#include <memory>
#include <string>

#include "shell/common/crash_reporter/crash_reporter_crashpad.h"
#include "third_party/crashpad/crashpad/client/crashpad_client.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace crash_reporter {

class CrashReporterWin : public CrashReporterCrashpad {
 public:
  static CrashReporterWin* GetInstance();
#if defined(_WIN64)
  static void SetUnhandledExceptionFilter();
#endif

  void Init(const std::string& submit_url,
            const base::FilePath& crashes_dir,
            bool upload_to_server,
            bool skip_system_crash_handler,
            bool rate_limit,
            bool compress) override;
  void SetUploadParameters() override;

  crashpad::CrashpadClient& GetCrashpadClient();

 private:
  friend struct base::DefaultSingletonTraits<CrashReporterWin>;
  CrashReporterWin();
  ~CrashReporterWin() override;

  void UpdatePipeName();

  crashpad::CrashpadClient crashpad_client_;

  DISALLOW_COPY_AND_ASSIGN(CrashReporterWin);
};

}  // namespace crash_reporter

#endif  // SHELL_COMMON_CRASH_REPORTER_CRASH_REPORTER_WIN_H_
