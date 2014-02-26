// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/crash_reporter/crash_reporter_linux.h"

#include <sys/time.h>
#include <unistd.h>

#include "base/debug/crash_logging.h"
#include "base/files/file_path.h"
#include "base/linux_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/memory.h"
#include "base/memory/singleton.h"
#include "vendor/breakpad/src/client/linux/handler/exception_handler.h"
#include "vendor/breakpad/src/common/linux/linux_libc_support.h"

using google_breakpad::ExceptionHandler;
using google_breakpad::MinidumpDescriptor;

namespace crash_reporter {

namespace {

static const size_t kDistroSize = 128;

// Define a preferred limit on minidump sizes, because Crash Server currently
// throws away any larger than 1.2MB (1.2 * 1024 * 1024).  A value of -1 means
// no limit.
static const off_t kMaxMinidumpFileSize = 1258291;

uint64_t g_process_start_time = 0;
pid_t g_pid = 0;
ExceptionHandler* g_breakpad = NULL;

// The following helper functions are for calculating uptime.

// Converts a struct timeval to milliseconds.
uint64_t timeval_to_ms(struct timeval *tv) {
  uint64_t ret = tv->tv_sec;  // Avoid overflow by explicitly using a uint64_t.
  ret *= 1000;
  ret += tv->tv_usec / 1000;
  return ret;
}

void SetProcessStartTime() {
  // Set the base process start time value.
  struct timeval tv;
  if (!gettimeofday(&tv, NULL))
    g_process_start_time = timeval_to_ms(&tv);
  else
    g_process_start_time = 0;
}

// Populates the passed in allocated string and its size with the distro of
// the crashing process.
// The passed string is expected to be at least kDistroSize bytes long.
void PopulateDistro(char* distro, size_t* distro_len_param) {
  size_t distro_len = std::min(my_strlen(base::g_linux_distro), kDistroSize);
  memcpy(distro, base::g_linux_distro, distro_len);
  if (distro_len_param)
    *distro_len_param = distro_len;
}

#if defined(ADDRESS_SANITIZER)
extern "C"
void __asan_set_error_report_callback(void (*cb)(const char*));

extern "C"
void AsanLinuxBreakpadCallback(const char* report) {
  // Send minidump here.
  g_breakpad->SimulateSignalDelivery(SIGKILL);
}
#endif

}  // namespace

CrashReporterLinux::CrashReporterLinux() {
  SetProcessStartTime();
  g_pid = getpid();

  // Make base::g_linux_distro work.
  base::SetLinuxDistro(base::GetLinuxDistro());
}

CrashReporterLinux::~CrashReporterLinux() {
}

void CrashReporterLinux::InitBreakpad(const std::string& product_name,
                                      const std::string& version,
                                      const std::string& company_name,
                                      const std::string& submit_url,
                                      bool auto_submit,
                                      bool skip_system_crash_handler) {
  EnableCrashDumping();

#if defined(ADDRESS_SANITIZER)
  // Register the callback for AddressSanitizer error reporting.
  __asan_set_error_report_callback(AsanLinuxBreakpadCallback);
#endif

  for (StringMap::const_iterator iter = upload_parameters_.begin();
       iter != upload_parameters_.end(); ++iter)
    crash_keys_.SetKeyValue(iter->first.c_str(), iter->second.c_str());
}

void CrashReporterLinux::SetUploadParameters() {
  upload_parameters_["platform"] = "linux";
}

void CrashReporterLinux::EnableCrashDumping() {
  base::FilePath tmp_path("/tmp");
  PathService::Get(base::DIR_TEMP, &tmp_path);

  base::FilePath dumps_path(tmp_path);
  DCHECK(!g_breakpad);
  MinidumpDescriptor minidump_descriptor(dumps_path.value());
  minidump_descriptor.set_size_limit(kMaxMinidumpFileSize);

  g_breakpad = new ExceptionHandler(
      minidump_descriptor,
      NULL,
      CrashDone,
      this,
      true,  // Install handlers.
      -1);   // Server file descriptor. -1 for in-process.
}

bool CrashReporterLinux::CrashDone(const MinidumpDescriptor& minidump,
                                   void* context,
                                   const bool succeeded) {
  CrashReporterLinux* self = static_cast<CrashReporterLinux*>(context);

  // WARNING: this code runs in a compromised context. It may not call into
  // libc nor allocate memory normally.
  if (!succeeded) {
    const char msg[] = "Failed to generate minidump.";
    WriteLog(msg, sizeof(msg) - 1);
    return false;
  }

  DCHECK(!minidump.IsFD());

  BreakpadInfo info = {0};
  info.filename = minidump.path();
  info.fd = minidump.fd();
#if defined(ADDRESS_SANITIZER)
  google_breakpad::PageAllocator allocator;
  const size_t log_path_len = my_strlen(minidump.path());
  char* log_path = reinterpret_cast<char*>(allocator.Alloc(log_path_len + 1));
  my_memcpy(log_path, minidump.path(), log_path_len);
  my_memcpy(log_path + log_path_len - 4, ".log", 4);
  log_path[log_path_len] = '\0';
  info.log_filename = log_path;
#endif
  // TODO(zcbenz): Set the correct process_type here.
  info.process_type = "browser";
  info.process_type_length = 7;
  info.distro = base::g_linux_distro;
  info.distro_length = my_strlen(base::g_linux_distro);
  info.upload = true;
  info.process_start_time = g_process_start_time;
  info.oom_size = base::g_oom_size;
  info.pid = g_pid;
  info.crash_keys = &self->crash_keys_;
  HandleCrashDump(info);
  return true;
}

// static
CrashReporterLinux* CrashReporterLinux::GetInstance() {
  return Singleton<CrashReporterLinux>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterLinux::GetInstance();
}

}  // namespace crash_reporter
