// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_WIN_H_
#define ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_WIN_H_

#include "base/compiler_specific.h"
#include "common/crash_reporter/crash_reporter.h"

template <typename T> struct DefaultSingletonTraits;

namespace crash_reporter {

class CrashReporterWin : public CrashReporter {
 public:
  static CrashReporterWin* GetInstance();

  virtual void InitBreakpad(const std::string& product_name,
                            const std::string& version,
                            const std::string& company_name,
                            const std::string& submit_url,
                            bool auto_submit,
                            bool skip_system_crash_handler) OVERRIDE;
  virtual void SetUploadParameters() OVERRIDE;

 private:
  friend struct DefaultSingletonTraits<CrashReporterWin>;

  CrashReporterWin();
  virtual ~CrashReporterWin();

  DISALLOW_COPY_AND_ASSIGN(CrashReporterWin);
};

}  // namespace crash_reporter

#endif  // ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_WIN_H_
