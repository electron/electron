// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_bindings.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/browser.h"
#include "atom/common/api/locker.h"
#include "atom/common/application_info.h"
#include "atom/common/atom_version.h"
#include "atom/common/heap_snapshot.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/logging.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/process/process_metrics_iocounters.h"
#include "base/system/sys_info.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/chrome_version.h"
#include "native_mate/dictionary.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/global_memory_dump.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"

// Must be the last in the includes list, otherwise the definition of chromium
// macros conflicts with node macros.
#include "atom/common/node_includes.h"

namespace atom {

namespace {

// Dummy class type that used for crashing the program.
struct DummyClass {
  bool crash;
};

// Called when there is a fatal error in V8, we just crash the process here so
// we can get the stack trace.
void FatalErrorCallback(const char* location, const char* message) {
  LOG(ERROR) << "Fatal error in V8: " << location << " " << message;
  AtomBindings::Crash();
}

}  // namespace

AtomBindings::AtomBindings(uv_loop_t* loop) {
  uv_async_init(loop, &call_next_tick_async_, OnCallNextTick);
  call_next_tick_async_.data = this;
  metrics_ = base::ProcessMetrics::CreateCurrentProcessMetrics();
}

AtomBindings::~AtomBindings() {
  uv_close(reinterpret_cast<uv_handle_t*>(&call_next_tick_async_), nullptr);
}

// static
void AtomBindings::BindProcess(v8::Isolate* isolate,
                               mate::Dictionary* process,
                               base::ProcessMetrics* metrics) {
  // These bindings are shared between sandboxed & unsandboxed renderers
  process->SetMethod("crash", &Crash);
  process->SetMethod("hang", &Hang);
  process->SetMethod("log", &Log);
  process->SetMethod("getCreationTime", &GetCreationTime);
  process->SetMethod("getHeapStatistics", &GetHeapStatistics);
  process->SetMethod("getProcessMemoryInfo", &GetProcessMemoryInfo);
  process->SetMethod("getSystemMemoryInfo", &GetSystemMemoryInfo);
  process->SetMethod("getIOCounters", &GetIOCounters);
  process->SetMethod("getCPUUsage", base::Bind(&AtomBindings::GetCPUUsage,
                                               base::Unretained(metrics)));

#if defined(MAS_BUILD)
  process->SetReadOnly("mas", true);
#endif

#if defined(OS_WIN)
  if (IsRunningInDesktopBridge())
    process->SetReadOnly("windowsStore", true);
#endif
}

void AtomBindings::BindTo(v8::Isolate* isolate, v8::Local<v8::Object> process) {
  isolate->SetFatalErrorHandler(FatalErrorCallback);

  mate::Dictionary dict(isolate, process);
  BindProcess(isolate, &dict, metrics_.get());

  dict.SetMethod("takeHeapSnapshot", &TakeHeapSnapshot);
#if defined(OS_POSIX)
  dict.SetMethod("setFdLimit", &base::IncreaseFdLimitTo);
#endif
  dict.SetMethod("activateUvLoop", base::Bind(&AtomBindings::ActivateUVLoop,
                                              base::Unretained(this)));

  mate::Dictionary versions;
  if (dict.Get("versions", &versions)) {
    versions.SetReadOnly(ATOM_PROJECT_NAME, ATOM_VERSION_STRING);
    versions.SetReadOnly("chrome", CHROME_VERSION_STRING);
  }
}

void AtomBindings::EnvironmentDestroyed(node::Environment* env) {
  auto it =
      std::find(pending_next_ticks_.begin(), pending_next_ticks_.end(), env);
  if (it != pending_next_ticks_.end())
    pending_next_ticks_.erase(it);
}

void AtomBindings::ActivateUVLoop(v8::Isolate* isolate) {
  node::Environment* env = node::Environment::GetCurrent(isolate);
  if (std::find(pending_next_ticks_.begin(), pending_next_ticks_.end(), env) !=
      pending_next_ticks_.end())
    return;

  pending_next_ticks_.push_back(env);
  uv_async_send(&call_next_tick_async_);
}

// static
void AtomBindings::OnCallNextTick(uv_async_t* handle) {
  AtomBindings* self = static_cast<AtomBindings*>(handle->data);
  for (std::list<node::Environment*>::const_iterator it =
           self->pending_next_ticks_.begin();
       it != self->pending_next_ticks_.end(); ++it) {
    node::Environment* env = *it;
    mate::Locker locker(env->isolate());
    v8::Context::Scope context_scope(env->context());
    node::InternalCallbackScope scope(
        env, v8::Local<v8::Object>(), {0, 0},
        node::InternalCallbackScope::kAllowEmptyResource);
  }

  self->pending_next_ticks_.clear();
}

// static
void AtomBindings::Log(const base::string16& message) {
  std::cout << message << std::flush;
}

// static
void AtomBindings::Crash() {
  static_cast<DummyClass*>(nullptr)->crash = true;
}

// static
void AtomBindings::Hang() {
  for (;;)
    base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(1));
}

// static
v8::Local<v8::Value> AtomBindings::GetHeapStatistics(v8::Isolate* isolate) {
  v8::HeapStatistics v8_heap_stats;
  isolate->GetHeapStatistics(&v8_heap_stats);

  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("totalHeapSize",
           static_cast<double>(v8_heap_stats.total_heap_size() >> 10));
  dict.Set(
      "totalHeapSizeExecutable",
      static_cast<double>(v8_heap_stats.total_heap_size_executable() >> 10));
  dict.Set("totalPhysicalSize",
           static_cast<double>(v8_heap_stats.total_physical_size() >> 10));
  dict.Set("totalAvailableSize",
           static_cast<double>(v8_heap_stats.total_available_size() >> 10));
  dict.Set("usedHeapSize",
           static_cast<double>(v8_heap_stats.used_heap_size() >> 10));
  dict.Set("heapSizeLimit",
           static_cast<double>(v8_heap_stats.heap_size_limit() >> 10));
  dict.Set("mallocedMemory",
           static_cast<double>(v8_heap_stats.malloced_memory() >> 10));
  dict.Set("peakMallocedMemory",
           static_cast<double>(v8_heap_stats.peak_malloced_memory() >> 10));
  dict.Set("doesZapGarbage",
           static_cast<bool>(v8_heap_stats.does_zap_garbage()));

  return dict.GetHandle();
}

// static
v8::Local<v8::Value> AtomBindings::GetCreationTime(v8::Isolate* isolate) {
  auto timeValue = base::Process::Current().CreationTime();
  if (timeValue.is_null()) {
    return v8::Null(isolate);
  }
  double jsTime = timeValue.ToJsTime();
  return v8::Number::New(isolate, jsTime);
}

// static
v8::Local<v8::Value> AtomBindings::GetSystemMemoryInfo(v8::Isolate* isolate,
                                                       mate::Arguments* args) {
  base::SystemMemoryInfoKB mem_info;
  if (!base::GetSystemMemoryInfo(&mem_info)) {
    args->ThrowError("Unable to retrieve system memory information");
    return v8::Undefined(isolate);
  }

  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("total", mem_info.total);

  // See Chromium's "base/process/process_metrics.h" for an explanation.
  int free =
#if defined(OS_WIN)
      mem_info.avail_phys;
#else
      mem_info.free;
#endif
  dict.Set("free", free);

  // NB: These return bogus values on macOS
#if !defined(OS_MACOSX)
  dict.Set("swapTotal", mem_info.swap_total);
  dict.Set("swapFree", mem_info.swap_free);
#endif

  return dict.GetHandle();
}

// static
v8::Local<v8::Promise> AtomBindings::GetProcessMemoryInfo(
    v8::Isolate* isolate) {
  util::Promise promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (mate::Locker::IsBrowserProcess() && !Browser::Get()->is_ready()) {
    promise.RejectWithErrorMessage(
        "Memory Info is available only after app ready");
    return handle;
  }

  v8::Global<v8::Context> context(isolate, isolate->GetCurrentContext());
  memory_instrumentation::MemoryInstrumentation::GetInstance()
      ->RequestGlobalDumpForPid(
          base::GetCurrentProcId(), std::vector<std::string>(),
          base::BindOnce(&AtomBindings::DidReceiveMemoryDump,
                         std::move(context), std::move(promise)));
  return handle;
}

// static
void AtomBindings::DidReceiveMemoryDump(
    v8::Global<v8::Context> context,
    util::Promise promise,
    bool success,
    std::unique_ptr<memory_instrumentation::GlobalMemoryDump> global_dump) {
  v8::Isolate* isolate = promise.isolate();
  mate::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope script_scope(isolate,
                                   v8::MicrotasksScope::kRunMicrotasks);
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(isolate, context));

  if (!success) {
    promise.RejectWithErrorMessage("Failed to create memory dump");
    return;
  }

  bool resolved = false;
  for (const memory_instrumentation::GlobalMemoryDump::ProcessDump& dump :
       global_dump->process_dumps()) {
    if (base::GetCurrentProcId() == dump.pid()) {
      mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
      const auto& osdump = dump.os_dump();
#if defined(OS_LINUX) || defined(OS_WIN)
      dict.Set("residentSet", osdump.resident_set_kb);
#endif
      dict.Set("private", osdump.private_footprint_kb);
      dict.Set("shared", osdump.shared_footprint_kb);
      promise.Resolve(dict.GetHandle());
      resolved = true;
      break;
    }
  }
  if (!resolved) {
    promise.RejectWithErrorMessage(
        R"(Failed to find current process memory details in memory dump)");
  }
}

// static
v8::Local<v8::Value> AtomBindings::GetCPUUsage(base::ProcessMetrics* metrics,
                                               v8::Isolate* isolate) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  int processor_count = base::SysInfo::NumberOfProcessors();
  dict.Set("percentCPUUsage",
           metrics->GetPlatformIndependentCPUUsage() / processor_count);

  // NB: This will throw NOTIMPLEMENTED() on Windows
  // For backwards compatibility, we'll return 0
#if !defined(OS_WIN)
  dict.Set("idleWakeupsPerSecond", metrics->GetIdleWakeupsPerSecond());
#else
  dict.Set("idleWakeupsPerSecond", 0);
#endif

  return dict.GetHandle();
}

// static
v8::Local<v8::Value> AtomBindings::GetIOCounters(v8::Isolate* isolate) {
  auto metrics = base::ProcessMetrics::CreateCurrentProcessMetrics();
  base::IoCounters io_counters;
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);

  if (metrics->GetIOCounters(&io_counters)) {
    dict.Set("readOperationCount", io_counters.ReadOperationCount);
    dict.Set("writeOperationCount", io_counters.WriteOperationCount);
    dict.Set("otherOperationCount", io_counters.OtherOperationCount);
    dict.Set("readTransferCount", io_counters.ReadTransferCount);
    dict.Set("writeTransferCount", io_counters.WriteTransferCount);
    dict.Set("otherTransferCount", io_counters.OtherTransferCount);
  }

  return dict.GetHandle();
}

// static
bool AtomBindings::TakeHeapSnapshot(v8::Isolate* isolate,
                                    const base::FilePath& file_path) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  base::File file(file_path,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);

  return atom::TakeHeapSnapshot(isolate, &file);
}

}  // namespace atom
