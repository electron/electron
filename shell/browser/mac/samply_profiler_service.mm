// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/mac/samply_profiler_service.h"

#include <servers/bootstrap.h>
#include <unistd.h>

#include "base/apple/bundle_locations.h"
#include "base/apple/mach_logging.h"
#include "base/apple/scoped_mach_port.h"
#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/numerics/byte_conversions.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "shell/common/mac/samply_profiler_client.h"

namespace electron {

namespace {

SamplyProfilerService* g_instance = nullptr;

// Returns the default path to the samply executable in the app bundle
base::FilePath GetDefaultSamplyPath() {
  base::FilePath framework_path = base::apple::FrameworkBundlePath();
  return framework_path.Append("Helpers").Append("samply");
}

// Returns the default output path for the profile.
base::FilePath GetDefaultOutputPath() {
  base::FilePath temp_dir;
  if (!base::GetTempDir(&temp_dir)) {
    temp_dir = base::FilePath("/tmp");
  }
  return temp_dir.Append("electron_profile.json.gz");
}

}  // namespace

// static
SamplyProfilerService* SamplyProfilerService::GetInstance() {
  return g_instance;
}

// static
bool SamplyProfilerService::Initialize() {
  VLOG(1) << "SamplyProfilerService::Initialize() called, PID=" << getpid();

  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch("enable-samply-profiling")) {
    return false;
  }

  if (g_instance) {
    VLOG(1) << "SamplyProfilerService: Already initialized, enabled="
            << g_instance->IsEnabled();
    return g_instance->IsEnabled();
  }

  base::FilePath samply_path;
  if (command_line->HasSwitch("samply-path")) {
    samply_path = command_line->GetSwitchValuePath("samply-path");
  } else {
    samply_path = GetDefaultSamplyPath();
  }

  if (!base::PathExists(samply_path)) {
    LOG(ERROR) << "samply not found at: " << samply_path;
    return false;
  }

  base::FilePath output_path;
  if (command_line->HasSwitch("samply-output-path")) {
    output_path = command_line->GetSwitchValuePath("samply-output-path");
  } else {
    output_path = GetDefaultOutputPath();
  }

  VLOG(1) << "SamplyProfilerService: Creating new instance...";
  g_instance = new SamplyProfilerService(samply_path, output_path);
  VLOG(1) << "SamplyProfilerService: Created, enabled="
          << g_instance->IsEnabled()
          << ", port=" << g_instance->GetProfilerPort();
  return g_instance->IsEnabled();
}

// static
void SamplyProfilerService::Shutdown() {
  if (g_instance) {
    delete g_instance;
    g_instance = nullptr;
  }
}

SamplyProfilerService::SamplyProfilerService(const base::FilePath& samply_path,
                                             const base::FilePath& output_path)
    : samply_path_(samply_path), output_path_(output_path) {
  mach_port_t port;
  kern_return_t kr =
      mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "mach_port_allocate for profiler";
    return;
  }
  receive_port_.reset(port);

  mach_port_t send_right;
  mach_msg_type_name_t acquired_type;
  kr = mach_port_extract_right(mach_task_self(), receive_port_.get(),
                               MACH_MSG_TYPE_MAKE_SEND, &send_right,
                               &acquired_type);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "mach_port_extract_right for profiler";
    return;
  }
  send_right_.reset(send_right);

  int pipe_fds[2];
  if (pipe(pipe_fds) != 0) {
    PLOG(ERROR) << "pipe for samply handshake";
    return;
  }

  int read_fd = pipe_fds[0];
  int write_fd = pipe_fds[1];

  base::LaunchOptions options;
  options.fds_to_remap.emplace_back(read_fd, read_fd);

  std::vector<std::string> argv = {
      samply_path_.value(), "record",
      "--handshake-fd",     base::StringPrintf("%d", read_fd),
      "--output",           output_path_.value()};

  samply_process_ = base::LaunchProcess(argv, options);
  close(read_fd);

  if (!samply_process_.IsValid()) {
    LOG(ERROR) << "Failed to launch samply";
    close(write_fd);
    return;
  }

  if (!PerformHandshake(write_fd)) {
    LOG(ERROR) << "Handshake with samply failed";
    samply_process_.Terminate(0, false);
    return;
  }

  ConnectSelfToProfiler();

  is_enabled_ = true;
  VLOG(1) << "Samply profiling enabled, PID: " << samply_process_.Pid();
}

SamplyProfilerService::~SamplyProfilerService() {
  // Release our send right - this triggers MACH_NOTIFY_NO_SENDERS in samply
  // when all other clients have disconnected, causing it to save and exit
  send_right_.reset();

  if (samply_process_.IsValid()) {
    // Send SIGINT to samply to request a graceful shutdown and
    // save the profile.
    VLOG(1) << "Sending SIGINT to samply to save profile...";
    kill(samply_process_.Pid(), SIGINT);

    // Wait briefly for samply to save the profile and exit.
    int exit_code;
    if (!samply_process_.WaitForExitWithTimeout(base::Seconds(5), &exit_code)) {
      LOG(WARNING) << "Samply did not exit in time, sending SIGTERM";
      kill(samply_process_.Pid(), SIGTERM);
      samply_process_.WaitForExitWithTimeout(base::Seconds(2), &exit_code);
    }
    VLOG(1) << "Samply exited with code: " << exit_code;
  }
}

bool SamplyProfilerService::PerformHandshake(int write_fd) {
  // Generate a random token for authentication
  uint64_t token = base::RandUint64();

  // Generate a unique service name
  std::string service_name = base::StringPrintf("org.electron.samply.%d.%llu",
                                                getpid(), base::RandUint64());

  // Register the receive port with the bootstrap server
  mach_port_t local_bootstrap_port;
  kern_return_t kr = task_get_special_port(
      mach_task_self(), TASK_BOOTSTRAP_PORT, &local_bootstrap_port);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "task_get_special_port";
    close(write_fd);
    return false;
  }

  // Extract a send right to register with bootstrap
  mach_port_t service_port;
  mach_msg_type_name_t acquired_type;
  kr = mach_port_extract_right(mach_task_self(), receive_port_.get(),
                               MACH_MSG_TYPE_MAKE_SEND, &service_port,
                               &acquired_type);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "mach_port_extract_right for service";
    close(write_fd);
    return false;
  }

  // Register with bootstrap server
  // bootstrap_register is deprecated but there's no modern replacement for
  // this functionality. We use it to allow samply to look up our port.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  kr =
      bootstrap_register(local_bootstrap_port,
                         const_cast<char*>(service_name.c_str()), service_port);
#pragma clang diagnostic pop
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "bootstrap_register";
    close(write_fd);
    mach_port_deallocate(mach_task_self(), service_port);
    return false;
  }

  if (write(write_fd, &token, sizeof(token)) != sizeof(token)) {
    PLOG(ERROR) << "write token";
    close(write_fd);
    return false;
  }

  uint32_t name_len = static_cast<uint32_t>(service_name.size());
  if (write(write_fd, &name_len, sizeof(name_len)) != sizeof(name_len)) {
    PLOG(ERROR) << "write name_len";
    close(write_fd);
    return false;
  }

  if (write(write_fd, service_name.c_str(), name_len) !=
      static_cast<ssize_t>(name_len)) {
    PLOG(ERROR) << "write service_name";
    close(write_fd);
    return false;
  }

  close(write_fd);

  // Wait for samply to send its check-in message
  CheckInMessageBuffer check_in = {};
  check_in.header.msgh_size = sizeof(CheckInMessageBuffer);
  check_in.header.msgh_local_port = receive_port_.get();

  kr = mach_msg(&check_in.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0,
                sizeof(CheckInMessageBuffer), receive_port_.get(),
                10000,  // 10 second timeout
                MACH_PORT_NULL);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "mach_msg receive check-in from samply";
    return false;
  }

  VLOG(1) << "Received check-in message: size=" << check_in.header.msgh_size
          << " remote_port=" << check_in.header.msgh_remote_port
          << " id=" << check_in.header.msgh_id;

  // Validate message size before accessing fields.
  if (check_in.header.msgh_size < kCheckInMessageMinSize) {
    LOG(ERROR) << "Check-in message too small, size="
               << check_in.header.msgh_size
               << " expected >= " << kCheckInMessageMinSize;
    return false;
  }

  // Extract the token from the received message at known offset.
  uint64_t received_token = base::U64FromNativeEndian(
      base::span(check_in.bytes)
          .subspan<kCheckInTokenOffset, sizeof(uint64_t)>());

  if (received_token != token) {
    LOG(ERROR) << "Invalid token in samply check-in message";
    return false;
  }

  // Send reply containing the receive right
  // After this, samply owns the receive right and we only keep a send right
  CheckInReply reply = {};
  // Use MACH_MSG_TYPE_MOVE_SEND because samply created a regular send right
  // (via MACH_MSG_TYPE_MAKE_SEND), not a send-once right.
  reply.header.msgh_bits =
      MACH_MSGH_BITS_COMPLEX | MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND, 0);
  reply.header.msgh_size = sizeof(CheckInReply);
  reply.header.msgh_remote_port = check_in.header.msgh_remote_port;
  reply.header.msgh_local_port = MACH_PORT_NULL;
  reply.header.msgh_id = check_in.header.msgh_id + 100;

  reply.body.msgh_descriptor_count = 1;

  reply.port.name = receive_port_.release();  // Transfer ownership to samply
  reply.port.disposition = MACH_MSG_TYPE_MOVE_RECEIVE;
  reply.port.type = MACH_MSG_PORT_DESCRIPTOR;

  kr = mach_msg(&reply.header, MACH_SEND_MSG, sizeof(CheckInReply), 0,
                MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "mach_msg send check-in reply to samply";
    return false;
  }

  VLOG(1) << "Handshake with samply completed successfully";
  return true;
}

void SamplyProfilerService::ConnectSelfToProfiler() {
  if (!send_right_.is_valid()) {
    LOG(WARNING) << "No send right available for self-connection";
    return;
  }

  if (ConnectToSamplyProfiler(send_right_.get())) {
    VLOG(1) << "Browser process connected to samply profiler";
  } else {
    LOG(WARNING) << "Failed to connect browser process to samply profiler";
  }
}

}  // namespace electron
