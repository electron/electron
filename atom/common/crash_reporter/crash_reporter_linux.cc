// Copyright (c) 2014 GitHub, Inc.
// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/crash_reporter_linux.h"

#include <sys/time.h>
#include <unistd.h>

#include <string>

#include "base/debug/crash_logging.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/linux_util.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/process/memory.h"
#include "base/strings/stringprintf.h"
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

}  // namespace

CrashReporterLinux::CrashReporterLinux()
    : process_start_time_(0),
      pid_(getpid()) {
  // Set the base process start time value.
  struct timeval tv;
  if (!gettimeofday(&tv, NULL)) {
    uint64_t ret = tv.tv_sec;
    ret *= 1000;
    ret += tv.tv_usec / 1000;
    process_start_time_ = ret;
  }

  // Make base::g_linux_distro work.
  base::SetLinuxDistro(base::GetLinuxDistro());
}

CrashReporterLinux::~CrashReporterLinux() {
}

void CrashReporterLinux::InitBreakpad(const std::string& product_name,
                                      const std::string& version,
                                      const std::string& company_name,
                                      const std::string& submit_url,
                                      const std::string& temp_dir,
                                      bool auto_submit,
                                      bool skip_system_crash_handler) {
  EnableCrashDumping(product_name, temp_dir);

  crash_keys_.SetKeyValue("prod", ATOM_PRODUCT_NAME);
  crash_keys_.SetKeyValue("ver", version.c_str());
  upload_url_ = submit_url;

  for (StringMap::const_iterator iter = upload_parameters_.begin();
       iter != upload_parameters_.end(); ++iter)
    crash_keys_.SetKeyValue(iter->first.c_str(), iter->second.c_str());
}

void CrashReporterLinux::SetUploadParameters() {
  upload_parameters_["platform"] = "linux";
}

void CrashReporterLinux::EnableCrashDumping(const std::string& product_name,
                                            const std::string& temp_dir) {
  base::FilePath dumps_path = GetCrashesDirectory(product_name, temp_dir);
  base::CreateDirectory(dumps_path);

  std::string log_file = base::StringPrintf(
      "%s/%s", dumps_path.value().c_str(), "uploads.log");
  strncpy(g_crash_log_path, log_file.c_str(), sizeof(g_crash_log_path));

  MinidumpDescriptor minidump_descriptor(dumps_path.value());
  minidump_descriptor.set_size_limit(kMaxMinidumpFileSize);

  breakpad_.reset(new ExceptionHandler(
      minidump_descriptor,
      NULL,
      CrashDone,
      this,
      true,  // Install handlers.
      -1));
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
  info.distro = base::g_linux_distro;
  info.distro_length = my_strlen(base::g_linux_distro);
  info.upload = true;
  info.process_start_time = self->process_start_time_;
  info.oom_size = base::g_oom_size;
  info.pid = self->pid_;
  info.upload_url = self->upload_url_.c_str();
  info.crash_keys = &self->crash_keys_;
  HandleCrashDump(info);
  return true;
}

// static
CrashReporterLinux* CrashReporterLinux::GetInstance() {
  return base::Singleton<CrashReporterLinux>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterLinux::GetInstance();
}

}  // namespace crash_reporter
