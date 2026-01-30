// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_MAC_SAMPLY_PROFILER_CLIENT_H_
#define ELECTRON_SHELL_COMMON_MAC_SAMPLY_PROFILER_CLIENT_H_

#include <mach/mach.h>

namespace electron {

// Attempts to connect this process to the samply profiler.
// Should be called early in process startup.
// Uses MachPortRendezvous to get the profiler port from the parent process.
void MaybeConnectToSamplyProfiler();

// Connects this process to the samply profiler using the given send right.
// The send right is borrowed (not consumed).
// Returns true if connection was successful.
bool ConnectToSamplyProfiler(mach_port_t profiler_port);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_MAC_SAMPLY_PROFILER_CLIENT_H_
