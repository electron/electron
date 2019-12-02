// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "orderfile_instrumentation.h"

#include <time.h>
#include <unistd.h>

#include <mach/mach_time.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "anchor_functions.h"
#include "base/files/file.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"

// Must be applied to all functions within this file.
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

namespace orderfile {

namespace {
// Constants used for StartDelayedDump().
constexpr int kDelayInSeconds = 30;
constexpr int kInitialDelayInSeconds = kPhases == 1 ? kDelayInSeconds : 5;

// These are large overestimates, which is not an issue, as the data is
// allocated in .bss, and on linux doesn't take any actual memory when it's not
// touched.
constexpr size_t kBitfieldSize = 1 << 22;
constexpr size_t kMaxTextSizeInBytes = kBitfieldSize * (4 * 32);
constexpr size_t kMaxElements = 1 << 20;

// Data required to log reached offsets.
struct LogData {
  std::atomic<uint32_t> offsets[kBitfieldSize];
  std::atomic<size_t> ordered_offsets[kMaxElements];
  std::atomic<size_t> index;
};

LogData g_data[kPhases];
std::atomic<int> g_data_index;

// |RecordAddress()| adds an element to a concurrent bitset and to a concurrent
// append-only list of offsets.
//
// Ordering:
// Two consecutive calls to |RecordAddress()| from the same thread will be
// ordered in the same way in the result, as written by
// |StopAndDumpToFile()|. The result will contain exactly one instance of each
// unique offset relative to |kStartOfText| passed to |RecordAddress()|.
//
// Implementation:
// The "set" part is implemented with a bitfield, |g_offset|. The insertion
// order is recorded in |g_ordered_offsets|.
// This is not a class to make sure there isn't a static constructor, as it
// would cause issue with an instrumented static constructor calling this code.
//
// Limitations:
// - Only records offsets to addresses between |kStartOfText| and |kEndOfText|.
// - Capacity of the set is limited by |kMaxElements|.
// - Some insertions at the end of collection may be lost.

// Records that |address| has been reached, if recording is enabled.
// To avoid infinite recursion, this *must* *never* call any instrumented
// function, unless |Disable()| is called first.
template <bool for_testing>
__attribute__((always_inline, no_instrument_function)) void RecordAddress(
    size_t address) {
  int index = g_data_index.load(std::memory_order_relaxed);
  if (index >= kPhases)
    return;

  const size_t start = for_testing ? kStartOfTextForTesting : kStartOfText;
  const size_t end = for_testing ? kEndOfTextForTesting : kEndOfText;
  if (UNLIKELY(address < start || address > end)) {
    Disable();
    // If the start and end addresses are set incorrectly, this code path is
    // likely happening during a static initializer. Logging at this time is
    // prone to deadlock. By crashing immediately we at least have a chance to
    // get a stack trace from the system to give some clue about the nature of
    // the problem.
    IMMEDIATE_CRASH();
  }

  size_t offset = address - start;
  static_assert(sizeof(int) == 4,
                "Collection and processing code assumes that sizeof(int) == 4");
  size_t offset_index = offset / 4;

  auto* offsets = g_data[index].offsets;
  // Atomically set the corresponding bit in the array.
  std::atomic<uint32_t>* element = offsets + (offset_index / 32);
  // First, a racy check. This saves a CAS if the bit is already set, and
  // allows the cache line to remain shared acoss CPUs in this case.
  uint32_t value = element->load(std::memory_order_relaxed);
  uint32_t mask = 1 << (offset_index % 32);
  if (value & mask)
    return;

  auto before = element->fetch_or(mask, std::memory_order_relaxed);
  if (before & mask)
    return;

  // We were the first one to set the element, record it in the ordered
  // elements list.
  // Use relaxed ordering, as the value is not published, or used for
  // synchronization.
  auto* ordered_offsets = g_data[index].ordered_offsets;
  auto& ordered_offsets_index = g_data[index].index;
  size_t insertion_index =
      ordered_offsets_index.fetch_add(1, std::memory_order_relaxed);
  if (UNLIKELY(insertion_index >= kMaxElements)) {
    Disable();
    LOG(FATAL) << "Too many reached offsets";
  }
  ordered_offsets[insertion_index].store(offset, std::memory_order_relaxed);
}

NO_INSTRUMENT_FUNCTION bool DumpToFile(const base::FilePath& path,
                                       const LogData& data) {
  auto file =
      base::File(path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    PLOG(ERROR) << "Could not open " << path;
    return false;
  }

  if (data.index == 0) {
    LOG(ERROR) << "No entries to dump";
    return false;
  }

  size_t count = data.index - 1;
  for (size_t i = 0; i < count; i++) {
    // |g_ordered_offsets| is initialized to 0, so a 0 in the middle of it
    // indicates a case where the index was incremented, but the write is not
    // visible in this thread yet. Safe to skip, also because the function at
    // the start of text is never called.
    auto offset = data.ordered_offsets[i].load(std::memory_order_relaxed);
    if (!offset)
      continue;
    auto offset_str = base::StringPrintf("%" PRIuS "\n", offset);
    if (file.WriteAtCurrentPos(offset_str.c_str(),
                               static_cast<int>(offset_str.size())) < 0) {
      // If the file could be opened, but writing has failed, it's likely that
      // data was partially written. Producing incomplete profiling data would
      // lead to a poorly performing orderfile, but might not be otherwised
      // noticed. So we crash instead.
      LOG(FATAL) << "Error writing profile data";
    }
  }
  return true;
}

// Stops recording, and outputs the data to |path|.
NO_INSTRUMENT_FUNCTION void StopAndDumpToFile(int pid,
                                              uint64_t start_ns_since_epoch,
                                              const std::string& tag) {
  Disable();

  for (int phase = 0; phase < kPhases; phase++) {
    std::string tag_str;
    if (!tag.empty())
      tag_str = base::StringPrintf("%s-", tag.c_str());
    auto path = base::StringPrintf(
        "/Users/nornagon/Desktop/profile-hitmap-%s%d-%" PRIu64 ".txt_%d",
        tag_str.c_str(), pid, start_ns_since_epoch, phase);
    if (!DumpToFile(base::FilePath(path), g_data[phase])) {
      LOG(ERROR) << "Problem with dump " << phase << " (" << tag << ")";
    }
  }
}

}  // namespace

NO_INSTRUMENT_FUNCTION bool Disable() {
  auto old_phase = g_data_index.exchange(kPhases, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  return old_phase != kPhases;
}

NO_INSTRUMENT_FUNCTION void SanityChecks() {
  CHECK_LT(kEndOfText - kStartOfText, kMaxTextSizeInBytes);
  CHECK(IsOrderingSane());
}

NO_INSTRUMENT_FUNCTION bool SwitchToNextPhaseOrDump(
    int pid,
    uint64_t start_ns_since_epoch) {
  int before = g_data_index.fetch_add(1, std::memory_order_relaxed);
  if (before + 1 == kPhases) {
    StopAndDumpToFile(pid, start_ns_since_epoch, "");
    return true;
  }
  return false;
}

NO_INSTRUMENT_FUNCTION void StartDelayedDump() {
  // Using std::thread and not using base::TimeTicks() in order to to not call
  // too many base:: symbols that would pollute the reached symbol dumps.
  uint64_t start_ns_since_epoch = mach_absolute_time();
  // TODO: eh... mach_absolute_time isn't really nanosecs, but whatever.
  int pid = getpid();

  std::thread([pid, start_ns_since_epoch]() {
    sleep(kInitialDelayInSeconds);
    while (!SwitchToNextPhaseOrDump(pid, start_ns_since_epoch))
      sleep(kDelayInSeconds);
  }).detach();
}

NO_INSTRUMENT_FUNCTION void Dump(const std::string& tag) {
  // As profiling has been disabled, none of the uses of ::base symbols below
  // will enter the symbol dump.
  StopAndDumpToFile(
      getpid(), (base::Time::Now() - base::Time::UnixEpoch()).InNanoseconds(),
      tag);
}

NO_INSTRUMENT_FUNCTION void ResetForTesting() {
  Disable();
  g_data_index = 0;
  for (int i = 0; i < kPhases; i++) {
    memset(reinterpret_cast<uint32_t*>(g_data[i].offsets), 0,
           sizeof(uint32_t) * kBitfieldSize);
    memset(reinterpret_cast<uint32_t*>(g_data[i].ordered_offsets), 0,
           sizeof(uint32_t) * kMaxElements);
    g_data[i].index.store(0);
  }
}

NO_INSTRUMENT_FUNCTION void RecordAddressForTesting(size_t address) {
  return RecordAddress<true>(address);
}

NO_INSTRUMENT_FUNCTION std::vector<size_t> GetOrderedOffsetsForTesting() {
  std::vector<size_t> result;
  size_t max_index = g_data[0].index.load(std::memory_order_relaxed);
  for (size_t i = 0; i < max_index; ++i) {
    auto value = g_data[0].ordered_offsets[i].load(std::memory_order_relaxed);
    if (value)
      result.push_back(value);
  }
  return result;
}

}  // namespace orderfile

extern "C" {

NO_INSTRUMENT_FUNCTION void __cyg_profile_func_enter_bare() {
  orderfile::RecordAddress<false>(
      reinterpret_cast<size_t>(__builtin_return_address(0)));
}

}  // extern "C"
