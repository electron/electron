// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/crash_reporter.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "common/atom_version.h"
#import "vendor/breakpad/src/client/apple/Framework/BreakpadDefines.h"
#import "vendor/breakpad/src/client/mac/Framework/Breakpad.h"

namespace crash_reporter {

namespace {

class ScopedCrashReporter {
 public:
  ScopedCrashReporter() {
    NSMutableDictionary* parameters =
        [NSMutableDictionary dictionaryWithCapacity:4];
    [parameters setValue:@"Atom-Shell" forKey:@BREAKPAD_PRODUCT];
    [parameters setValue:@"GitHub, Inc" forKey:@BREAKPAD_VENDOR];
    [parameters setValue:@"0" forKey:@BREAKPAD_REPORT_INTERVAL];
    [parameters setValue:@ATOM_VERSION_STRING forKey:@BREAKPAD_VERSION];
    // Use my server as /dev/null for now.
    [parameters setValue:@"http://54.249.141.255" forKey:@BREAKPAD_URL];

    breakpad_ = BreakpadCreate(parameters);
  }

  ~ScopedCrashReporter() { if (breakpad_) BreakpadRelease(breakpad_); }

  void SetKey(const std::string& key, const std::string& value) {
    BreakpadSetKeyValue(breakpad_,
                        base::SysUTF8ToNSString(key),
                        base::SysUTF8ToNSString(value));
  }

  static ScopedCrashReporter* Get() {
    if (g_scoped_crash_reporter_ == NULL)
      g_scoped_crash_reporter_ = new ScopedCrashReporter();
    return g_scoped_crash_reporter_;
  }

 private:
  BreakpadRef breakpad_;

  static ScopedCrashReporter* g_scoped_crash_reporter_;

  DISALLOW_COPY_AND_ASSIGN(ScopedCrashReporter);
};

ScopedCrashReporter* ScopedCrashReporter::g_scoped_crash_reporter_ = NULL;

}  // namespace

// static
void CrashReporter::SetCompanyName(const std::string& name) {
  ScopedCrashReporter::Get()->SetKey(BREAKPAD_VENDOR, name);
}

// static
void CrashReporter::SetSubmissionURL(const std::string& url) {
  ScopedCrashReporter::Get()->SetKey(BREAKPAD_URL, url);
}

// static
void CrashReporter::SetAutoSubmit(bool yes) {
  ScopedCrashReporter::Get()->SetKey(BREAKPAD_SKIP_CONFIRM, yes ? "YES" : "NO");
}

}  // namespace crash_reporter
