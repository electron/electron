// Copyright (c) 2014 GitHub, Inc.
// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_CRASH_REPORTER_LINUX_CRASH_DUMP_HANDLER_H_
#define SHELL_COMMON_CRASH_REPORTER_LINUX_CRASH_DUMP_HANDLER_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "base/macros.h"
#include "breakpad/src/common/simple_string_dictionary.h"
#include "components/crash/core/common/crash_key_internal.h"

namespace crash_reporter {

// The size of the iovec used to transfer crash data from a child back to the
// browser.
#if !defined(ADDRESS_SANITIZER)
const size_t kCrashIovSize = 6;
#else
// Additional field to pass the AddressSanitizer log to the crash handler.
const size_t kCrashIovSize = 7;
#endif

// BreakpadInfo describes a crash report.
// The minidump information can either be contained in a file descriptor (fd) or
// in a file (whose path is in filename).
struct BreakpadInfo {
  int fd;                // File descriptor to the Breakpad dump data.
  const char* filename;  // Path to the Breakpad dump data.
#if defined(ADDRESS_SANITIZER)
  const char* log_filename;     // Path to the ASan log file.
  const char* asan_report_str;  // ASan report.
  unsigned asan_report_length;  // Length of |asan_report_length|.
#endif
  const char* process_type;      // Process type, e.g. "renderer".
  unsigned process_type_length;  // Length of |process_type|.
  const char* distro;            // Linux distro string.
  unsigned distro_length;        // Length of |distro|.
  bool upload;                   // Whether to upload or save crash dump.
  uint64_t process_start_time;   // Uptime of the crashing process.
  size_t oom_size;               // Amount of memory requested if OOM.
  uint64_t pid;                  // PID where applicable.
  crash_reporter::internal::TransitionalCrashKeyStorage* crash_keys;
};

void HandleCrashDump(const BreakpadInfo& info);

void SetUploadURL(const char* url);

size_t WriteLog(const char* buf, size_t nbytes);
size_t WriteNewline();

// Global variable storing the path of upload log.
extern char g_crash_log_path[256];

}  // namespace crash_reporter

#endif  // SHELL_COMMON_CRASH_REPORTER_LINUX_CRASH_DUMP_HANDLER_H_
