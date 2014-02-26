// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_CRASH_REPORTER_LINUX_CRASH_DUMP_HANDLER_H_
#define ATOM_COMMON_CRASH_REPORTER_LINUX_CRASH_DUMP_HANDLER_H_

#include "base/basictypes.h"
#include "vendor/breakpad/src/common/simple_string_dictionary.h"

namespace crash_reporter {

typedef google_breakpad::NonAllocatingMap<256, 256, 64> CrashKeyStorage;

// BreakpadInfo describes a crash report.
// The minidump information can either be contained in a file descriptor (fd) or
// in a file (whose path is in filename).
struct BreakpadInfo {
  int fd;                          // File descriptor to the Breakpad dump data.
  const char* filename;            // Path to the Breakpad dump data.
#if defined(ADDRESS_SANITIZER)
  const char* log_filename;        // Path to the ASan log file.
  const char* asan_report_str;     // ASan report.
  unsigned asan_report_length;     // Length of |asan_report_length|.
#endif
  const char* distro;              // Linux distro string.
  unsigned distro_length;          // Length of |distro|.
  bool upload;                     // Whether to upload or save crash dump.
  uint64_t process_start_time;     // Uptime of the crashing process.
  size_t oom_size;                 // Amount of memory requested if OOM.
  uint64_t pid;                    // PID where applicable.
  CrashKeyStorage* crash_keys;
};

void HandleCrashDump(const BreakpadInfo& info);

size_t WriteLog(const char* buf, size_t nbytes);

}  // namespace crash_reporter

#endif  // ATOM_COMMON_CRASH_REPORTER_LINUX_CRASH_DUMP_HANDLER_H_
