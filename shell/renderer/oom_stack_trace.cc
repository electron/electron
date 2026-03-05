// Copyright (c) 2026 Anysphere, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/oom_stack_trace.h"

#include <atomic>
#include <cstdio>
#include <string>

#include "electron/mas.h"
#include "shell/common/crash_keys.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "v8/include/v8-exception.h"
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

// Runs at the next V8 safe point after the heap limit was hit.
// At a safe point, all frames have deoptimization data available,
// so CurrentStackTrace won't FATAL on optimized frames.
void CaptureStackOnInterrupt(v8::Isolate* isolate, void* data) {
  if (!isolate->InContext()) {
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
    fprintf(stderr, "\n<--- JS stacktrace (captured at safe point) --->\n%s\n",
            js_stack.c_str());
    fflush(stderr);
  }
}

size_t NearHeapLimitCallback(void* data,
                             size_t current_heap_limit,
                             size_t initial_heap_limit) {
  auto* isolate = static_cast<v8::Isolate*>(data);

  if (g_is_in_oom.exchange(true)) {
    return current_heap_limit;
  }

  v8::HeapStatistics stats;
  isolate->GetHeapStatistics(&stats);
  fprintf(stderr,
          "\n<--- Near heap limit --->\n"
          "Heap: used=%.1fMB limit=%.1fMB\n",
          stats.used_heap_size() / 1048576.0,
          stats.heap_size_limit() / 1048576.0);
  fflush(stderr);

#if !IS_MAS_BUILD()
  crash_keys::SetCrashKey(
      "electron.v8-oom.stack",
      absl::StrFormat("Heap: used=%.1fMB limit=%.1fMB (stack pending)",
                      stats.used_heap_size() / 1048576.0,
                      stats.heap_size_limit() / 1048576.0));
#endif

  // Request an interrupt to capture the JS stack at the next safe point,
  // where optimized frames have deoptimization data available.
  // CurrentStackTrace is unsafe to call directly here because V8 may
  // FATAL on optimized frames missing deopt info during OOM.
  isolate->RequestInterrupt(CaptureStackOnInterrupt, nullptr);

  // Remove ourselves and bump the limit to give V8 room to reach a safe
  // point where the interrupt can fire and capture the stack trace.
  isolate->RemoveNearHeapLimitCallback(NearHeapLimitCallback, 0);

  constexpr size_t kHeapBump = 20 * 1024 * 1024;
  return current_heap_limit + kHeapBump;
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
