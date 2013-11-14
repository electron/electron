// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_H_
#define ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_H_

#include <map>
#include <string>

#include "base/basictypes.h"

namespace crash_reporter {

class CrashReporter {
 public:
  static CrashReporter* GetInstance();

  void Start(std::string product_name,
             const std::string& company_name,
             const std::string& submit_url,
             bool auto_submit,
             bool skip_system_crash_handler);

 protected:
  CrashReporter();
  virtual ~CrashReporter();

  virtual void InitBreakpad(const std::string& product_name,
                            const std::string& version,
                            const std::string& company_name,
                            const std::string& submit_url,
                            bool auto_submit,
                            bool skip_system_crash_handler) = 0;
  virtual void SetUploadParameters();

  typedef std::map<std::string, std::string> StringMap;
  StringMap upload_parameters_;
  bool is_browser_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CrashReporter);
};

}  // namespace crash_reporter

#endif  // ATOM_COMMON_CRASH_REPORTER_CRASH_REPORTER_H_
