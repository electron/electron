// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/javascript_environment.h"

#include <string>

#include "base/command_line.h"
#include "base/message_loop/message_loop_current.h"
#include "base/task/thread_pool/initialization_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/content_switches.h"
#include "gin/array_buffer.h"
#include "gin/v8_initializer.h"
#include "shell/browser/microtasks_runner.h"
#include "shell/common/node_includes.h"
#include "tracing/trace_event.h"

namespace electron {

JavascriptEnvironment::JavascriptEnvironment(uv_loop_t* event_loop)
    : isolate_(Initialize(event_loop)),
      isolate_holder_(base::ThreadTaskRunnerHandle::Get(),
                      gin::IsolateHolder::kSingleThread,
                      gin::IsolateHolder::kAllowAtomicsWait,
                      gin::IsolateHolder::IsolateType::kUtility,
                      gin::IsolateHolder::IsolateCreationMode::kNormal,
                      isolate_),
      isolate_scope_(isolate_),
      locker_(isolate_),
      handle_scope_(isolate_),
      context_(isolate_, node::NewContext(isolate_)),
      context_scope_(v8::Local<v8::Context>::New(isolate_, context_)) {}

JavascriptEnvironment::~JavascriptEnvironment() = default;

v8::Isolate* JavascriptEnvironment::Initialize(uv_loop_t* event_loop) {
  auto* cmd = base::CommandLine::ForCurrentProcess();

  // --js-flags.
  std::string js_flags = cmd->GetSwitchValueASCII(switches::kJavaScriptFlags);
  if (!js_flags.empty())
    v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  // The V8Platform of gin relies on Chromium's task schedule, which has not
  // been started at this point, so we have to rely on Node's V8Platform.
  auto* tracing_agent = node::CreateAgent();
  auto* tracing_controller = tracing_agent->GetTracingController();
  node::tracing::TraceEventHelper::SetAgent(tracing_agent);
  platform_ = node::CreatePlatform(
      base::RecommendedMaxNumberOfThreadsInThreadGroup(3, 8, 0.1, 0),
      tracing_controller);

  v8::V8::InitializePlatform(platform_);
  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
                                 gin::ArrayBufferAllocator::SharedInstance(),
                                 nullptr /* external_reference_table */,
                                 false /* create_v8_platform */);

  v8::Isolate* isolate = v8::Isolate::Allocate();
  platform_->RegisterIsolate(isolate, event_loop);

  return isolate;
}

void JavascriptEnvironment::OnMessageLoopCreated() {
  DCHECK(!microtasks_runner_);
  microtasks_runner_.reset(new MicrotasksRunner(isolate()));
  base::MessageLoopCurrent::Get()->AddTaskObserver(microtasks_runner_.get());
}

void JavascriptEnvironment::OnMessageLoopDestroying() {
  DCHECK(microtasks_runner_);
  base::MessageLoopCurrent::Get()->RemoveTaskObserver(microtasks_runner_.get());
  platform_->DrainTasks(isolate_);
  platform_->UnregisterIsolate(isolate_);
}

NodeEnvironment::NodeEnvironment(node::Environment* env) : env_(env) {}

NodeEnvironment::~NodeEnvironment() {
  node::FreeEnvironment(env_);
}

}  // namespace electron
