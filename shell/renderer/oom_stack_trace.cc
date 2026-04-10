// Copyright (c) 2026 Anysphere, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/oom_stack_trace.h"

#include <atomic>
#include <limits>
#include <string>
#include <string_view>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "electron/mas.h"
#include "shell/common/crash_keys.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-internal.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-statistics.h"

namespace electron {

namespace {

std::atomic<bool> g_is_in_oom{false};
std::atomic<v8::Isolate*> g_registered_isolate{nullptr};

std::string FormatStackTrace(v8::Isolate* isolate,
                             v8::Local<v8::StackTrace> stack) {
  std::string result;
  int frame_count = stack->GetFrameCount();
  for (int i = 0; i < frame_count; i++) {
    v8::Local<v8::StackFrame> frame = stack->GetFrame(isolate, i);

    v8::Local<v8::String> function_name = frame->GetFunctionName();
    v8::Local<v8::String> script_name = frame->GetScriptName();
    int line = frame->GetLineNumber();
    int col = frame->GetColumn();

    std::string func_str = "(anonymous)";
    if (!function_name.IsEmpty()) {
      v8::String::Utf8Value utf8(isolate, function_name);
      if (*utf8) {
        func_str = *utf8;
      }
    }

    std::string script_str = "<unknown>";
    if (!script_name.IsEmpty()) {
      v8::String::Utf8Value utf8(isolate, script_name);
      if (*utf8) {
        script_str = *utf8;
      }
    }

    absl::StrAppendFormat(&result, "    #%d %s (%s:%d:%d)\n", i, func_str,
                          script_str, line, col);
  }
  return result;
}

void CaptureStackOnInterrupt(v8::Isolate* isolate, void* data) {
  if (!isolate->InContext()) {
    return;
  }

  v8::HeapStatistics stats;
  isolate->GetHeapStatistics(&stats);
  constexpr size_t kMinHeadroom = 2 * 1024 * 1024;  // 2 MB
  if (stats.used_heap_size() + kMinHeadroom > stats.heap_size_limit()) {
    LOG(ERROR) << "Skipping JS stack capture: insufficient heap headroom";
    return;
  }

  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::StackTrace> stack =
      v8::StackTrace::CurrentStackTrace(isolate, 10);
  if (stack.IsEmpty() || stack->GetFrameCount() == 0) {
    return;
  }

  std::string js_stack = FormatStackTrace(isolate, stack);
  if (!js_stack.empty()) {
#if !IS_MAS_BUILD()
    crash_keys::SetCrashKey("electron.v8-oom.stack", js_stack);
#endif
    LOG(ERROR) << "\n<--- JS stacktrace (captured at safe point) --->\n"
               << js_stack;
  }
}

// V8's pointer compression cage limits the heap to kPtrComprCageReservationSize
// (~4 GB). After this callback returns a new limit, V8 clamps it to at most
// that cage size. If current_heap_limit is already near the ceiling the bump
// is effectively zero and the interrupt never fires. Fall back to heap info.
size_t NearHeapLimitCallback(void* data,
                             size_t current_heap_limit,
                             size_t initial_heap_limit) {
  auto* isolate = static_cast<v8::Isolate*>(data);

  if (g_is_in_oom.exchange(true)) {
    return current_heap_limit;
  }

  v8::HeapStatistics stats;
  isolate->GetHeapStatistics(&stats);
  std::string heap_info = absl::StrFormat("Heap: used=%.1fMB limit=%.1fMB",
                                          stats.used_heap_size() / 1048576.0,
                                          stats.heap_size_limit() / 1048576.0);
  LOG(ERROR) << "\n<--- Near heap limit --->\n" << heap_info;

#if !IS_MAS_BUILD()
  crash_keys::SetCrashKey("electron.v8-oom.stack",
                          heap_info + " (stack pending)");

  crash_keys::SetCrashKey("electron.v8-oom.heap.used",
                          base::NumberToString(stats.used_heap_size()));
  crash_keys::SetCrashKey("electron.v8-oom.heap.total",
                          base::NumberToString(stats.total_heap_size()));
  crash_keys::SetCrashKey("electron.v8-oom.heap.limit",
                          base::NumberToString(stats.heap_size_limit()));
  crash_keys::SetCrashKey("electron.v8-oom.heap.total_available",
                          base::NumberToString(stats.total_available_size()));
  crash_keys::SetCrashKey("electron.v8-oom.heap.total_physical",
                          base::NumberToString(stats.total_physical_size()));
  crash_keys::SetCrashKey("electron.v8-oom.heap.malloced_memory",
                          base::NumberToString(stats.malloced_memory()));
  crash_keys::SetCrashKey("electron.v8-oom.heap.external_memory",
                          base::NumberToString(stats.external_memory()));
  crash_keys::SetCrashKey(
      "electron.v8-oom.heap.native_contexts",
      base::NumberToString(stats.number_of_native_contexts()));
  crash_keys::SetCrashKey(
      "electron.v8-oom.heap.detached_contexts",
      base::NumberToString(stats.number_of_detached_contexts()));

  double utilization = static_cast<double>(stats.used_heap_size()) /
                       stats.heap_size_limit() * 100.0;
  crash_keys::SetCrashKey("electron.v8-oom.heap.utilization_pct",
                          absl::StrFormat("%.1f", utilization));

  v8::HeapSpaceStatistics space_stats;
  for (size_t i = 0; i < isolate->NumberOfHeapSpaces(); i++) {
    isolate->GetHeapSpaceStatistics(&space_stats, i);
    if (std::string_view(space_stats.space_name()) == "old_space") {
      crash_keys::SetCrashKey(
          "electron.v8-oom.old_space.used",
          base::NumberToString(space_stats.space_used_size()));
      crash_keys::SetCrashKey("electron.v8-oom.old_space.size",
                              base::NumberToString(space_stats.space_size()));
    } else if (std::string_view(space_stats.space_name()) ==
               "large_object_space") {
      crash_keys::SetCrashKey(
          "electron.v8-oom.lo_space.used",
          base::NumberToString(space_stats.space_used_size()));
      crash_keys::SetCrashKey("electron.v8-oom.lo_space.size",
                              base::NumberToString(space_stats.space_size()));
    }
  }
#endif

  isolate->RequestInterrupt(CaptureStackOnInterrupt, nullptr);
  isolate->RemoveNearHeapLimitCallback(NearHeapLimitCallback, 0);

  constexpr size_t kHeapBump = 20 * 1024 * 1024;
  size_t new_limit = current_heap_limit + kHeapBump;

#ifdef V8_COMPRESS_POINTERS
  constexpr size_t kCageLimit = v8::internal::kPtrComprCageReservationSize;
#else
  constexpr size_t kCageLimit = std::numeric_limits<size_t>::max();
#endif

  if (current_heap_limit >= kCageLimit - kHeapBump) {
#if !IS_MAS_BUILD()
    crash_keys::SetCrashKey("electron.v8-oom.stack",
                            heap_info + " (at cage limit, stack unavailable)");
#endif
    LOG(ERROR) << "Near V8 cage limit; stack trace capture may not succeed";
  }

  return new_limit;
}

}  // namespace

void RegisterOomStackTraceCallback(v8::Isolate* isolate) {
  v8::Isolate* expected = nullptr;
  if (!g_registered_isolate.compare_exchange_strong(expected, isolate)) {
    return;
  }
  isolate->AddNearHeapLimitCallback(NearHeapLimitCallback, isolate);
}

}  // namespace electron
