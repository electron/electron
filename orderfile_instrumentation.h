// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_ORDERFILE_ORDERFILE_INSTRUMENTATION_H_
#define BASE_ANDROID_ORDERFILE_ORDERFILE_INSTRUMENTATION_H_

#include <cstdint>
#include <vector>

namespace orderfile {
constexpr int kPhases = 1;

constexpr size_t kStartOfTextForTesting = 1000;
constexpr size_t kEndOfTextForTesting = kStartOfTextForTesting + 1000 * 1000;

// Stop recording. Returns false if recording was already disabled.
bool Disable();

// Switches to the next recording phase. If called from the last phase, dumps
// the data to disk, and returns |true|. |pid| is the current process pid, and
// |start_ns_since_epoch| the process start timestamp.
bool SwitchToNextPhaseOrDump(int pid, uint64_t start_ns_since_epoch);

// Starts a thread to dump instrumentation after a delay.
void StartDelayedDump();

// Dumps all information for the current process, annotating the dump file name
// with the given tag. Will disable instrumentation. Instrumentation must be
// disabled before this is called.
void Dump(const std::string& tag);

// Record an |address|, if recording is enabled. Only for testing.
void RecordAddressForTesting(size_t address);

// Record |callee_address, caller_address|, if recording is enabled.
// Only for testing.
void RecordAddressForTesting(size_t callee_address, size_t caller_address);

// Resets the state. Only for testing.
void ResetForTesting();

// Returns an ordered list of reached offsets. Only for testing.
std::vector<size_t> GetOrderedOffsetsForTesting();
}  // namespace orderfile

#endif  // BASE_ANDROID_ORDERFILE_ORDERFILE_INSTRUMENTATION_H_
