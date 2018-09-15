// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_bindings.h"

#include <algorithm>
#include <iostream>
#include <string>

#include "atom/common/api/locker.h"
#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/node_includes.h"
#include "base/logging.h"
#include "base/process/process_info.h"
#include "base/process/process_metrics_iocounters.h"
#include "base/sys_info.h"
#include "native_mate/dictionary.h"

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

void AtomBindings::BindTo(v8::Isolate* isolate, v8::Local<v8::Object> process) {
  isolate->SetFatalErrorHandler(FatalErrorCallback);

  mate::Dictionary dict(isolate, process);
  dict.SetMethod("crash", &AtomBindings::Crash);
  dict.SetMethod("hang", &Hang);
  dict.SetMethod("log", &Log);
  dict.SetMethod("getHeapStatistics", &GetHeapStatistics);
  dict.SetMethod("getCreationTime", &GetCreationTime);
  dict.SetMethod("getSystemMemoryInfo", &GetSystemMemoryInfo);
  dict.SetMethod("getCPUUsage", base::Bind(&AtomBindings::GetCPUUsage,
                                           base::Unretained(metrics_.get())));
  dict.SetMethod("getIOCounters", &GetIOCounters);
#if defined(OS_POSIX)
  dict.SetMethod("setFdLimit", &base::IncreaseFdLimitTo);
#endif
  dict.SetMethod("activateUvLoop", base::Bind(&AtomBindings::ActivateUVLoop,
                                              base::Unretained(this)));

#if defined(MAS_BUILD)
  dict.Set("mas", true);
#endif

  mate::Dictionary versions;
  if (dict.Get("versions", &versions)) {
    // TODO(kevinsawicki): Make read-only in 2.0 to match node
    versions.Set(ATOM_PROJECT_NAME, ATOM_VERSION_STRING);
    versions.Set("chrome", CHROME_VERSION_STRING);
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
  auto timeValue = base::CurrentProcessInfo::CreationTime();
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

}  // namespace atom
