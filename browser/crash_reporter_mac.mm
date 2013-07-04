// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/crash_reporter.h"

#import <Quincy/BWQuincyManager.h>

#include "base/strings/sys_string_conversions.h"

namespace crash_reporter {

// static
void CrashReporter::SetCompanyName(const std::string& name) {
  BWQuincyManager *manager = [BWQuincyManager sharedQuincyManager];
  [manager setCompanyName:base::SysUTF8ToNSString(name)];
}

// static
void CrashReporter::SetSubmissionURL(const std::string& url) {
  BWQuincyManager *manager = [BWQuincyManager sharedQuincyManager];
  [manager setSubmissionURL:base::SysUTF8ToNSString(name)];
  return v8::Undefined();
}

// static
void CrashReporter::SetAutoSubmit(bool yes) {
  BWQuincyManager *manager = [BWQuincyManager sharedQuincyManager];
  [manager setAutoSubmitCrashReport:yes];
}

}  // namespace crash_reporter
