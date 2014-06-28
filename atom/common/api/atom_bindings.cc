// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_bindings.h"

#include <string>

#include "atom/common/atom_version.h"
#include "atom/common/native_mate_converters/function_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/callback.h"
#include "base/logging.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace {

// Async handle to wake up uv loop.
uv_async_t g_next_tick_uv_handle;

// Async handle to execute the stored v8 callback.
uv_async_t g_callback_uv_handle;

// Stored v8 callback, to be called by the async handler.
base::Closure g_v8_callback;

// Dummy class type that used for crashing the program.
struct DummyClass { bool crash; };

// Async handler to call next process.nextTick callbacks.
void UvCallNextTick(uv_async_t* handle) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  node::Environment* env = node::Environment::GetCurrent(isolate);
  node::Environment::TickInfo* tick_info = env->tick_info();

  if (tick_info->in_tick())
    return;

  if (tick_info->length() == 0) {
    tick_info->set_index(0);
    return;
  }

  tick_info->set_in_tick(true);
  env->tick_callback_function()->Call(env->process_object(), 0, NULL);
  tick_info->set_in_tick(false);
}

// Async handler to execute the stored v8 callback.
void UvOnCallback(uv_async_t* handle) {
  g_v8_callback.Run();
}

// Called when there is a fatal error in V8, we just crash the process here so
// we can get the stack trace.
void FatalErrorCallback(const char* location, const char* message) {
  LOG(ERROR) << "Fatal error in V8: " << location << " " << message;
  static_cast<DummyClass*>(NULL)->crash = true;
}

v8::Handle<v8::Value> DumpStackFrame(v8::Isolate* isolate,
                                     v8::Handle<v8::StackFrame> stack_frame) {
  mate::Dictionary frame_dict(isolate);
  frame_dict.Set("line", stack_frame->GetLineNumber());
  frame_dict.Set("column", stack_frame->GetColumn());
  return mate::ConvertToV8(isolate, frame_dict);;
}

void Crash() {
  static_cast<DummyClass*>(NULL)->crash = true;
}

void ActivateUVLoop() {
  uv_async_send(&g_next_tick_uv_handle);
}

void Log(const base::string16& message) {
  logging::LogMessage("CONSOLE", 0, 0).stream() << message;
}

v8::Handle<v8::Value> GetCurrentStackTrace(v8::Isolate* isolate,
                                           int stack_limit) {
  v8::Local<v8::StackTrace> stack_trace = v8::StackTrace::CurrentStackTrace(
      isolate, stack_limit, v8::StackTrace::kDetailed);

  int frame_count = stack_trace->GetFrameCount();
  v8::Local<v8::Array> result = v8::Array::New(isolate, frame_count);
  for (int i = 0; i < frame_count; ++i)
    result->Set(i, DumpStackFrame(isolate, stack_trace->GetFrame(i)));

  return result;
}

void ScheduleCallback(const base::Closure& callback) {
  g_v8_callback = callback;
  uv_async_send(&g_callback_uv_handle);
}

}  // namespace


AtomBindings::AtomBindings() {
  uv_async_init(uv_default_loop(), &g_next_tick_uv_handle, UvCallNextTick);
  uv_async_init(uv_default_loop(), &g_callback_uv_handle, UvOnCallback);
  v8::V8::SetFatalErrorHandler(FatalErrorCallback);
}

AtomBindings::~AtomBindings() {
}

void AtomBindings::BindTo(v8::Isolate* isolate,
                          v8::Handle<v8::Object> process) {
  mate::Dictionary dict(isolate, process);
  dict.SetMethod("crash", &Crash);
  dict.SetMethod("activateUvLoop", &ActivateUVLoop);
  dict.SetMethod("log", &Log);
  dict.SetMethod("getCurrentStackTrace", &GetCurrentStackTrace);
  dict.SetMethod("scheduleCallback", &ScheduleCallback);

  v8::Handle<v8::Object> versions;
  if (dict.Get("versions", &versions))
    versions->Set(mate::StringToV8(isolate, "atom-shell"),
                  mate::StringToV8(isolate, ATOM_VERSION_STRING));
}

}  // namespace atom
