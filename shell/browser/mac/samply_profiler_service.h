// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MAC_SAMPLY_PROFILER_SERVICE_H_
#define ELECTRON_SHELL_BROWSER_MAC_SAMPLY_PROFILER_SERVICE_H_

#include <mach/mach.h>

#include <memory>
#include <string>

#include "base/apple/scoped_mach_port.h"
#include "base/files/file_path.h"
#include "base/process/process.h"
#include "shell/common/mac/samply_profiler_types.h"

namespace electron {

// Manages the samply profiler connection for the browser process.
//
// When profiling is enabled, this service:
// 1. Creates a Mach receive port for task connections
// 2. Launches the bundled samply process with --handshake-fd
// 3. Sends the receive right to samply via the handshake protocol
// 4. Provides the send right for child processes via MachPortRendezvous
// 5. Connects the browser process itself to the profiler
class SamplyProfilerService {
 public:
  // Returns the singleton instance. Returns nullptr if profiling is not
  // enabled.
  static SamplyProfilerService* GetInstance();

  // Initialize profiling. Must be called early in browser startup.
  // Returns true if profiling was successfully started.
  static bool Initialize();

  // Shutdown profiling and wait for samply to write the profile.
  static void Shutdown();

  SamplyProfilerService(const SamplyProfilerService&) = delete;
  SamplyProfilerService& operator=(const SamplyProfilerService&) = delete;

  bool IsEnabled() const { return is_enabled_; }

  // Returns a send right to the profiler port (for child process rendezvous).
  mach_port_t GetProfilerPort() const {
    return send_right_.is_valid() ? send_right_.get() : MACH_PORT_NULL;
  }

 private:
  SamplyProfilerService(const base::FilePath& samply_path,
                        const base::FilePath& output_path);
  ~SamplyProfilerService();

  // Perform the handshake with samply to transfer the receive right.
  bool PerformHandshake(int write_fd);

  // Connect browser process to the profiler.
  void ConnectSelfToProfiler();

  // The receive right for task connections
  // (owned temporarily during handshake)
  base::apple::ScopedMachReceiveRight receive_port_;

  // Send right to the profiler port
  base::apple::ScopedMachSendRight send_right_;

  // The samply process handle
  base::Process samply_process_;

  // Path to the samply executable
  base::FilePath samply_path_;

  // Path to where samply should save the profile
  base::FilePath output_path_;

  // Whether profiling is active
  bool is_enabled_ = false;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_MAC_SAMPLY_PROFILER_SERVICE_H_
