// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/javascript_environment.h"

#include <string>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/task_scheduler/initialization_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/content_switches.h"
#include "gin/array_buffer.h"
#include "gin/v8_initializer.h"

#include "atom/common/node_includes.h"
#include "tracing/trace_event.h"

namespace atom {

JavascriptEnvironment::JavascriptEnvironment()
    : initialized_(Initialize()),
      isolate_holder_(base::ThreadTaskRunnerHandle::Get(),
                      new AtomIsolateAllocator(this)),
      isolate_(isolate_holder_.isolate()),
      isolate_scope_(isolate_),
      locker_(isolate_),
      handle_scope_(isolate_),
      context_(isolate_, v8::Context::New(isolate_)),
      context_scope_(v8::Local<v8::Context>::New(isolate_, context_)) {
  // The assumption is made here that the isolate_holder_ holds a single isolate
  // if this is not the case or changes in the constructor above the logic
  // below must be rewritten
  node::InitializePrivatePropertiesOnIsolateData(isolate_data_);
}

JavascriptEnvironment::~JavascriptEnvironment() = default;

void JavascriptEnvironment::OnMessageLoopCreated() {}

void JavascriptEnvironment::OnMessageLoopDestroying() {}

bool JavascriptEnvironment::Initialize() {
  auto* cmd = base::CommandLine::ForCurrentProcess();

  // --js-flags.
  std::string js_flags = cmd->GetSwitchValueASCII(switches::kJavaScriptFlags);
  if (!js_flags.empty())
    v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  // The V8Platform of gin relies on Chromium's task schedule, which has not
  // been started at this point, so we have to rely on Node's V8Platform.
  auto* tracing_controller = new v8::TracingController();
  node::tracing::TraceEventHelper::SetTracingController(tracing_controller);
  platform_ = node::CreatePlatform(
      base::RecommendedMaxNumberOfThreadsInPool(3, 8, 0.1, 0),
      tracing_controller);
  v8::V8::InitializePlatform(platform_);
  gin::IsolateHolder::Initialize(
      gin::IsolateHolder::kNonStrictMode, gin::IsolateHolder::kStableV8Extras,
      gin::ArrayBufferAllocator::SharedInstance(),
      nullptr /* external_reference_table */, false /* create_v8_platform */);
  return true;
}

NodeEnvironment::NodeEnvironment(node::Environment* env) : env_(env) {}

NodeEnvironment::~NodeEnvironment() {
  node::FreeEnvironment(env_);
}

AtomIsolateAllocator::AtomIsolateAllocator(JavascriptEnvironment* env)
    : env_(env) {}

v8::Isolate* AtomIsolateAllocator::Allocate() {
  v8::Isolate* isolate = v8::Isolate::Allocate();
  // This is a cheatsy way to add the Isolate and it's IsolateData to the node
  // platform before it is ready
  env_->set_isolate_data(node::CreateIsolateData(
      isolate, uv_default_loop(), env_->platform(), true /* only_register */));
  return isolate;
}

}  // namespace atom
