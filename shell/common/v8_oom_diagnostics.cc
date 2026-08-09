// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/v8_oom_diagnostics.h"

#include <atomic>

#include "base/strings/safe_sprintf.h"
#include "components/crash/core/common/crash_key.h"
#include "electron/mas.h"
#include "v8/include/v8-callbacks.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-statistics.h"

namespace electron::v8_oom {

#if !IS_MAS_BUILD()

namespace {

// Whichever isolate hits an OOM first owns the keys. V8 also reports
// process-level allocation failures with no current isolate (possibly off
// the main thread); those claim under a sentinel.
char g_no_isolate;
std::atomic<const void*> g_owner{nullptr};

bool ClaimFor(v8::Isolate* isolate) {
  const void* current = isolate ? static_cast<const void*>(isolate)
                                : static_cast<const void*>(&g_no_isolate);
  const void* expected = nullptr;
  if (g_owner.compare_exchange_strong(expected, current)) {
    return true;
  }
  return expected == current;
}

using NumberKey = crash_reporter::CrashKeyString<24>;

void SetSize(NumberKey& key, size_t value) {
  char buf[24];
  base::strings::SafeSPrintf(buf, "%d", value);
  key.Set(buf);
}

}  // namespace

bool RecordHeapDiagnostics(v8::Isolate* isolate) {
  if (!isolate || !ClaimFor(isolate)) {
    return false;
  }

  static NumberKey heap_used_key("electron.v8-oom.heap.used");
  static NumberKey heap_total_key("electron.v8-oom.heap.total");
  static NumberKey heap_limit_key("electron.v8-oom.heap.limit");
  static NumberKey heap_available_key("electron.v8-oom.heap.total_available");
  static NumberKey heap_physical_key("electron.v8-oom.heap.total_physical");
  static NumberKey heap_malloced_key("electron.v8-oom.heap.malloced_memory");
  static NumberKey heap_external_key("electron.v8-oom.heap.external_memory");
  static NumberKey native_contexts_key("electron.v8-oom.heap.native_contexts");
  static NumberKey detached_contexts_key(
      "electron.v8-oom.heap.detached_contexts");
  static crash_reporter::CrashKeyString<8> utilization_key(
      "electron.v8-oom.heap.utilization_pct");
  static NumberKey old_space_used_key("electron.v8-oom.old_space.used");
  static NumberKey old_space_size_key("electron.v8-oom.old_space.size");
  static NumberKey lo_space_used_key("electron.v8-oom.lo_space.used");
  static NumberKey lo_space_size_key("electron.v8-oom.lo_space.size");

  v8::HeapStatistics stats;
  isolate->GetHeapStatistics(&stats);
  SetSize(heap_used_key, stats.used_heap_size());
  SetSize(heap_total_key, stats.total_heap_size());
  SetSize(heap_limit_key, stats.heap_size_limit());
  SetSize(heap_available_key, stats.total_available_size());
  SetSize(heap_physical_key, stats.total_physical_size());
  SetSize(heap_malloced_key, stats.malloced_memory());
  SetSize(heap_external_key, stats.external_memory());
  SetSize(native_contexts_key, stats.number_of_native_contexts());
  SetSize(detached_contexts_key, stats.number_of_detached_contexts());

  // Percentage in tenths, computed without floating point.
  size_t pct10 = stats.heap_size_limit() > 0
                     ? stats.used_heap_size() * 1000 / stats.heap_size_limit()
                     : 1000;
  char pct_buf[8];
  base::strings::SafeSPrintf(pct_buf, "%d.%d", pct10 / 10, pct10 % 10);
  utilization_key.Set(pct_buf);

  v8::HeapSpaceStatistics space_stats;
  for (size_t i = 0; i < isolate->NumberOfHeapSpaces(); i++) {
    isolate->GetHeapSpaceStatistics(&space_stats, i);
    std::string_view name(space_stats.space_name());
    if (name == "old_space") {
      SetSize(old_space_used_key, space_stats.space_used_size());
      SetSize(old_space_size_key, space_stats.space_size());
    } else if (name == "large_object_space") {
      SetSize(lo_space_used_key, space_stats.space_used_size());
      SetSize(lo_space_size_key, space_stats.space_size());
    }
  }
  return true;
}

void RecordErrorDetails(v8::Isolate* isolate,
                        const char* location,
                        const v8::OOMDetails& details) {
  if (!ClaimFor(isolate)) {
    return;
  }
  static crash_reporter::CrashKeyString<8> is_heap_oom_key(
      "electron.v8-oom.is_heap_oom");
  static crash_reporter::CrashKeyString<128> location_key(
      "electron.v8-oom.location");
  static crash_reporter::CrashKeyString<256> detail_key(
      "electron.v8-oom.detail");
  is_heap_oom_key.Set(details.is_heap_oom ? "1" : "0");
  if (location) {
    location_key.Set(location);
  }
  if (details.detail) {
    detail_key.Set(details.detail);
  }
}

void RecordJsStack(v8::Isolate* isolate, std::string_view stack) {
  if (!ClaimFor(isolate)) {
    return;
  }
  static crash_reporter::CrashKeyString<20320> stack_key(
      "electron.v8-oom.stack");
  stack_key.Set(stack);
}

#else

bool RecordHeapDiagnostics(v8::Isolate*) {
  return false;
}
void RecordErrorDetails(v8::Isolate*, const char*, const v8::OOMDetails&) {}
void RecordJsStack(v8::Isolate*, std::string_view) {}

#endif  // !IS_MAS_BUILD()

}  // namespace electron::v8_oom
