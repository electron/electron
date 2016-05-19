// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_bindings.h"

#include <algorithm>
#include <string>
#include <iostream>

#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/logging.h"
#include "base/process/process_metrics.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace {

// Dummy class type that used for crashing the program.
struct DummyClass { bool crash; };

void Crash() {
  static_cast<DummyClass*>(NULL)->crash = true;
}

void Hang() {
  for (;;)
    base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(1));
}

v8::Local<v8::Value> GetProcessMemoryInfo(v8::Isolate* isolate) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  std::unique_ptr<base::ProcessMetrics> metrics(
    base::ProcessMetrics::CreateCurrentProcessMetrics());

  dict.Set("workingSetSize",
    static_cast<double>(metrics->GetWorkingSetSize() >> 10));
  dict.Set("peakWorkingSetSize",
    static_cast<double>(metrics->GetPeakWorkingSetSize() >> 10));

  size_t private_bytes, shared_bytes;
  if (metrics->GetMemoryBytes(&private_bytes, &shared_bytes)) {
    dict.Set("privateBytes", static_cast<double>(private_bytes >> 10));
    dict.Set("sharedBytes", static_cast<double>(shared_bytes >> 10));
  }

  return dict.GetHandle();
}

v8::Local<v8::Value> GetSystemMemoryInfo(
    v8::Isolate* isolate,
    mate::Arguments* args) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  base::SystemMemoryInfoKB memInfo;

  if (!base::GetSystemMemoryInfo(&memInfo)) {
    args->ThrowError("Unable to retrieve system memory information");
    return v8::Undefined(isolate);
  }

  dict.Set("total", memInfo.total);
  dict.Set("free", memInfo.free);

  // NB: These return bogus values on OS X
#if !defined(OS_MACOSX)
  dict.Set("swapTotal", memInfo.swap_total);
  dict.Set("swapFree", memInfo.swap_free);
#endif

  return dict.GetHandle();
}

// Called when there is a fatal error in V8, we just crash the process here so
// we can get the stack trace.
void FatalErrorCallback(const char* location, const char* message) {
  LOG(ERROR) << "Fatal error in V8: " << location << " " << message;
  Crash();
}

void Log(const base::string16& message) {
  std::cout << message << std::flush;
}

}  // namespace


AtomBindings::AtomBindings() {
  uv_async_init(uv_default_loop(), &call_next_tick_async_, OnCallNextTick);
  call_next_tick_async_.data = this;
}

AtomBindings::~AtomBindings() {
}

void AtomBindings::BindTo(v8::Isolate* isolate,
                          v8::Local<v8::Object> process) {
  v8::V8::SetFatalErrorHandler(FatalErrorCallback);

  mate::Dictionary dict(isolate, process);
  dict.SetMethod("crash", &Crash);
  dict.SetMethod("hang", &Hang);
  dict.SetMethod("log", &Log);
  dict.SetMethod("getProcessMemoryInfo", &GetProcessMemoryInfo);
  dict.SetMethod("getSystemMemoryInfo", &GetSystemMemoryInfo);
#if defined(OS_POSIX)
  dict.SetMethod("setFdLimit", &base::SetFdLimit);
#endif
  dict.SetMethod("activateUvLoop",
      base::Bind(&AtomBindings::ActivateUVLoop, base::Unretained(this)));

#if defined(MAS_BUILD)
  dict.Set("mas", true);
#endif

  mate::Dictionary versions;
  if (dict.Get("versions", &versions)) {
    versions.Set(ATOM_PROJECT_NAME, ATOM_VERSION_STRING);
    versions.Set("atom-shell", ATOM_VERSION_STRING);  // For compatibility.
    versions.Set("chrome", CHROME_VERSION_STRING);
  }
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
    // KickNextTick, copied from node.cc:
    node::Environment::AsyncCallbackScope callback_scope(env);
    if (callback_scope.in_makecallback())
      continue;
    node::Environment::TickInfo* tick_info = env->tick_info();
    if (tick_info->length() == 0)
      env->isolate()->RunMicrotasks();
    v8::Local<v8::Object> process = env->process_object();
    if (tick_info->length() == 0)
      tick_info->set_index(0);
    env->tick_callback_function()->Call(process, 0, nullptr).IsEmpty();
  }

  self->pending_next_ticks_.clear();
}

}  // namespace atom
