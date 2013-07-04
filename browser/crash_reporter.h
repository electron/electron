// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_CRASH_REPORTER_H_
#define ATOM_BROWSER_CRASH_REPORTER_H_

#include <string>

#include "base/basictypes.h"

namespace crash_reporter {

class CrashReporter {
 public:
  static void SetCompanyName(const std::string& name);
  static void SetSubmissionURL(const std::string& url);
  static void SetAutoSubmit(bool yes);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CrashReporter);
};

}  // namespace crash_reporter

#endif  // ATOM_BROWSER_CRASH_REPORTER_H_
