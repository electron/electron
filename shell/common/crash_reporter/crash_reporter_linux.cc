// Copyright (c) 2014 GitHub, Inc.
// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/crash_reporter/crash_reporter_linux.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/linux_util.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/posix/global_descriptors.h"
#include "base/process/memory.h"
#include "base/threading/thread_restrictions.h"
#include "services/service_manager/embedder/descriptors.h"
#include "third_party/breakpad/breakpad/src/client/linux/crash_generation/crash_generation_client.h"
#include "third_party/breakpad/breakpad/src/client/linux/handler/exception_handler.h"
#include "third_party/breakpad/breakpad/src/common/linux/linux_libc_support.h"

#define IGNORE_RET(x) ignore_result(x)

using google_breakpad::ExceptionHandler;
using google_breakpad::MinidumpDescriptor;

namespace crash_reporter {

namespace {

// Define a preferred limit on minidump sizes, because Crash Server currently
// throws away any larger than 1.2MB (1.2 * 1024 * 1024).  A value of -1 means
// no limit.
static const off_t kMaxMinidumpFileSize = 1258291;

uint64_t g_process_start_time = 0;

}  // namespace

CrashReporterLinux::CrashReporterLinux() : pid_(getpid()) {
  // Set the base process start time value.
  struct timeval tv;
  if (!gettimeofday(&tv, nullptr)) {
    uint64_t ret = tv.tv_sec;
    ret *= 1000;
    ret += tv.tv_usec / 1000;
    g_process_start_time = ret;
  }

  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    // Make base::g_linux_distro work.
    base::SetLinuxDistro(base::GetLinuxDistro());
  }
}

CrashReporterLinux::~CrashReporterLinux() = default;

void CrashReporterLinux::Init(const std::string& submit_url,
                              const base::FilePath& crashes_dir,
                              bool upload_to_server,
                              bool skip_system_crash_handler,
                              bool rate_limit,
                              bool compress,
                              const StringMap& global_extra) {
  EnableCrashDumping(crashes_dir);
  crashes_dir_ = crashes_dir;

  SetUploadURL(submit_url.c_str());
  upload_to_server_ = upload_to_server;

  crash_keys_ = std::make_unique<CrashKeyStorage>();
  for (const auto& iter : upload_parameters_)
    crash_keys_->SetKeyValue(iter.first.c_str(), iter.second.c_str());
}

void CrashReporterLinux::InitInChild() {
  LOG(INFO) << "CRL::InitInChild " << process_type_;
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch("crashes-dir")) {
    EnableNonBrowserCrashDumping();
  }
}

void CrashReporterLinux::SetUploadParameters() {
  upload_parameters_["platform"] = "linux";
}

void CrashReporterLinux::SetUploadToServer(const bool upload_to_server) {
  upload_to_server_ = upload_to_server;
}

bool CrashReporterLinux::GetUploadToServer() {
  return upload_to_server_;
}

void CrashReporterLinux::AddExtraParameter(const std::string& key,
                                           const std::string& value) {}

void CrashReporterLinux::RemoveExtraParameter(const std::string& key) {}

base::FilePath CrashReporterLinux::GetCrashesDirectory() {
  return crashes_dir_;
}

// Non-Browser = Extension, Gpu, Plugins, Ppapi and Renderer
// Cribbed from //components/crash/core/app/breakpad_linux.cc
class NonBrowserCrashHandler : public google_breakpad::CrashGenerationClient {
 public:
  NonBrowserCrashHandler()
      : server_fd_(base::GlobalDescriptors::GetInstance()->Get(
            service_manager::kCrashDumpSignal)) {}

  ~NonBrowserCrashHandler() override {}

  bool RequestDump(const void* crash_context,
                   size_t crash_context_size) override {
    int fds[2] = {-1, -1};
    if (sys_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
      static const char msg[] = "Failed to create socket for crash dumping.\n";
      WriteLog(msg, sizeof(msg) - 1);
      return false;
    }

    // Start constructing the message to send to the browser.
    char b;                   // Dummy variable for sys_read below.
    const char* b_addr = &b;  // Get the address of |b| so we can create the
                              // expected /proc/[pid]/syscall content in the
                              // browser to convert namespace tids.

    // The length of the control message:
    static const unsigned kControlMsgSize = sizeof(int);
    static const unsigned kControlMsgSpaceSize = CMSG_SPACE(kControlMsgSize);
    static const unsigned kControlMsgLenSize = CMSG_LEN(kControlMsgSize);

    struct kernel_msghdr msg;
    my_memset(&msg, 0, sizeof(struct kernel_msghdr));
    struct kernel_iovec iov[kCrashIovSize];
    iov[0].iov_base = const_cast<void*>(crash_context);
    iov[0].iov_len = crash_context_size;
    iov[1].iov_base = &b_addr;
    iov[1].iov_len = sizeof(b_addr);
    iov[2].iov_base = &fds[0];
    iov[2].iov_len = sizeof(fds[0]);
    iov[3].iov_base = &g_process_start_time;
    iov[3].iov_len = sizeof(g_process_start_time);
    iov[4].iov_base = &base::g_oom_size;
    iov[4].iov_len = sizeof(base::g_oom_size);
    google_breakpad::SerializedNonAllocatingMap* serialized_map;
    iov[5].iov_len = crash_reporter::internal::GetCrashKeyStorage()->Serialize(
        const_cast<const google_breakpad::SerializedNonAllocatingMap**>(
            &serialized_map));
    iov[5].iov_base = serialized_map;
#if !defined(ADDRESS_SANITIZER)
    static_assert(5 == kCrashIovSize - 1, "kCrashIovSize should equal 6");
#else
    if (g_asan_report_str != nullptr) {
      iov[6].iov_base = const_cast<char*>(g_asan_report_str);
    } else {
      static char empty_asan_report[kMaxAsanReportSize + 1];
      iov[6].iov_base = empty_asan_report;
    }
    iov[6].iov_len = kMaxAsanReportSize + 1;
    static_assert(6 == kCrashIovSize - 1, "kCrashIovSize should equal 7");
#endif

    msg.msg_iov = iov;
    msg.msg_iovlen = kCrashIovSize;
    char cmsg[kControlMsgSpaceSize];
    my_memset(cmsg, 0, kControlMsgSpaceSize);
    msg.msg_control = cmsg;
    msg.msg_controllen = sizeof(cmsg);

    struct cmsghdr* hdr = CMSG_FIRSTHDR(&msg);
    hdr->cmsg_level = SOL_SOCKET;
    hdr->cmsg_type = SCM_RIGHTS;
    hdr->cmsg_len = kControlMsgLenSize;
    ((int*)CMSG_DATA(hdr))[0] = fds[1];

    if (HANDLE_EINTR(sys_sendmsg(server_fd_, &msg, 0)) < 0) {
      static const char errmsg[] = "Failed to tell parent about crash.\n";
      WriteLog(errmsg, sizeof(errmsg) - 1);
      IGNORE_RET(sys_close(fds[0]));
      IGNORE_RET(sys_close(fds[1]));
      return false;
    }
    IGNORE_RET(sys_close(fds[1]));

    if (HANDLE_EINTR(sys_read(fds[0], &b, 1)) != 1) {
      static const char errmsg[] = "Parent failed to complete crash dump.\n";
      WriteLog(errmsg, sizeof(errmsg) - 1);
    }
    IGNORE_RET(sys_close(fds[0]));

    return true;
  }

 private:
  // The pipe FD to the browser process, which will handle the crash dumping.
  const int server_fd_;

  DISALLOW_COPY_AND_ASSIGN(NonBrowserCrashHandler);
};

void CrashReporterLinux::EnableNonBrowserCrashDumping() {
  DCHECK(!breakpad_);
  breakpad_ = std::make_unique<ExceptionHandler>(
      MinidumpDescriptor("/tmp"),  // Unused but needed or Breakpad will assert.
      nullptr, nullptr, nullptr, true, -1);
  breakpad_->set_crash_generation_client(new NonBrowserCrashHandler());
}

void CrashReporterLinux::EnableCrashDumping(const base::FilePath& crashes_dir) {
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    base::CreateDirectory(crashes_dir);
  }
  std::string log_file = crashes_dir.Append("uploads.log").value();
  strncpy(g_crash_log_path, log_file.c_str(), sizeof(g_crash_log_path));

  MinidumpDescriptor minidump_descriptor(crashes_dir.value());
  minidump_descriptor.set_size_limit(kMaxMinidumpFileSize);

  breakpad_ = std::make_unique<ExceptionHandler>(minidump_descriptor, nullptr,
                                                 CrashDone, this,
                                                 true,  // Install handlers.
                                                 -1);
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
  info.upload = self->upload_to_server_;
  info.process_start_time = g_process_start_time;
  info.oom_size = base::g_oom_size;
  info.pid = self->pid_;
  // info.upload_url = self->upload_url_.c_str();
  info.crash_keys = self->crash_keys_.get();
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
