// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/crash_reporter_mac.h"

#include "base/mac/mac_util.h"
#include "base/memory/singleton.h"
#include "base/strings/sys_string_conversions.h"
#import "vendor/breakpad/src/client/apple/Framework/BreakpadDefines.h"

namespace crash_reporter {

CrashReporterMac::CrashReporterMac()
    : breakpad_(NULL) {
}

CrashReporterMac::~CrashReporterMac() {
  if (breakpad_ != NULL)
    BreakpadRelease(breakpad_);
}

void CrashReporterMac::InitBreakpad(const std::string& product_name,
                                    const std::string& version,
                                    const std::string& company_name,
                                    const std::string& submit_url,
                                    bool auto_submit,
                                    bool skip_system_crash_handler) {
  if (breakpad_ != NULL)
    BreakpadRelease(breakpad_);

  NSMutableDictionary* parameters =
      [NSMutableDictionary dictionaryWithCapacity:4];

  [parameters setValue:@ATOM_PRODUCT_NAME
                forKey:@BREAKPAD_PRODUCT];
  [parameters setValue:base::SysUTF8ToNSString(product_name)
                forKey:@BREAKPAD_PRODUCT_DISPLAY];
  [parameters setValue:base::SysUTF8ToNSString(version)
                forKey:@BREAKPAD_VERSION];
  [parameters setValue:base::SysUTF8ToNSString(company_name)
                forKey:@BREAKPAD_VENDOR];
  [parameters setValue:base::SysUTF8ToNSString(submit_url)
                forKey:@BREAKPAD_URL];
  [parameters setValue:(auto_submit ? @"YES" : @"NO")
                forKey:@BREAKPAD_SKIP_CONFIRM];
  [parameters setValue:(skip_system_crash_handler ? @"YES" : @"NO")
                forKey:@BREAKPAD_SEND_AND_EXIT];

  // Report all crashes (important for testing the crash reporter).
  [parameters setValue:@"0" forKey:@BREAKPAD_REPORT_INTERVAL];

  // Put dump files under "/tmp/ProductName Crashes".
  std::string dump_dir = "/tmp/" + product_name + " Crashes";
  [parameters setValue:base::SysUTF8ToNSString(dump_dir)
                forKey:@BREAKPAD_DUMP_DIRECTORY];

  // Temporarily run Breakpad in-process on 10.10 and later because APIs that
  // it depends on got broken (http://crbug.com/386208).
  // This can catch crashes in the browser process only.
  if (base::mac::IsOSYosemiteOrLater()) {
    [parameters setObject:[NSNumber numberWithBool:YES]
                   forKey:@BREAKPAD_IN_PROCESS];
  }

  breakpad_ = BreakpadCreate(parameters);
  if (!breakpad_) {
    LOG(ERROR) << "Failed to initialize breakpad";
    return;
  }

  for (StringMap::const_iterator iter = upload_parameters_.begin();
       iter != upload_parameters_.end(); ++iter) {
    BreakpadAddUploadParameter(breakpad_,
                               base::SysUTF8ToNSString(iter->first),
                               base::SysUTF8ToNSString(iter->second));
  }
}

void CrashReporterMac::SetUploadParameters() {
  upload_parameters_["platform"] = "darwin";
}

// static
CrashReporterMac* CrashReporterMac::GetInstance() {
  return Singleton<CrashReporterMac>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterMac::GetInstance();
}

}  // namespace crash_reporter
