// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_bindings.h"

#include <algorithm>
#include <string>

#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/logging.h"
#include "native_mate/callback.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace {

// Async handle to execute the stored v8 callback.
uv_async_t g_callback_uv_handle;

// Stored v8 callback, to be called by the async handler.
base::Closure g_v8_callback;

// Dummy class type that used for crashing the program.
struct DummyClass { bool crash; };

// Async handler to execute the stored v8 callback.
void UvOnCallback(uv_async_t* handle) {
  g_v8_callback.Run();
}

void Crash() {
  static_cast<DummyClass*>(NULL)->crash = true;
}

// Called when there is a fatal error in V8, we just crash the process here so
// we can get the stack trace.
void FatalErrorCallback(const char* location, const char* message) {
  LOG(ERROR) << "Fatal error in V8: " << location << " " << message;
  Crash();
}

void Log(const base::string16& message) {
  logging::LogMessage("CONSOLE", 0, 0).stream() << message;
}

void ScheduleCallback(const base::Closure& callback) {
  g_v8_callback = callback;
  uv_async_send(&g_callback_uv_handle);
}

}  // namespace


AtomBindings::AtomBindings() {
  uv_async_init(uv_default_loop(), &call_next_tick_async_, OnCallNextTick);
  call_next_tick_async_.data = this;

  uv_async_init(uv_default_loop(), &g_callback_uv_handle, UvOnCallback);
}

AtomBindings::~AtomBindings() {
}

void AtomBindings::BindTo(v8::Isolate* isolate,
                          v8::Handle<v8::Object> process) {
  v8::V8::SetFatalErrorHandler(FatalErrorCallback);

  mate::Dictionary dict(isolate, process);
  dict.SetMethod("crash", &Crash);
  dict.SetMethod("log", &Log);
  dict.SetMethod("scheduleCallback", &ScheduleCallback);
  dict.SetMethod("activateUvLoop",
      base::Bind(&AtomBindings::ActivateUVLoop, base::Unretained(this)));

  v8::Handle<v8::Object> versions;
  if (dict.Get("versions", &versions)) {
    versions->Set(mate::StringToV8(isolate, "atom-shell"),
                  mate::StringToV8(isolate, ATOM_VERSION_STRING));
    versions->Set(mate::StringToV8(isolate, "chrome"),
                  mate::StringToV8(isolate, CHROME_VERSION_STRING));
  }

  v8::Handle<v8::Value> event = mate::StringToV8(isolate, "BIND_DONE");
  node::MakeCallback(isolate, process, "emit", 1, &event);
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
    node::Environment::TickInfo* tick_info = env->tick_info();

    v8::Context::Scope context_scope(env->context());
    if (tick_info->in_tick())
      continue;

    if (tick_info->length() == 0) {
      tick_info->set_index(0);
      continue;
    }

    tick_info->set_in_tick(true);
    env->tick_callback_function()->Call(env->process_object(), 0, NULL);
    tick_info->set_in_tick(false);
  }

  self->pending_next_ticks_.clear();
}

}  // namespace atom
