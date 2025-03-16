// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/javascript_environment.h"

#include <memory>
#include <string>
#include <utility>

#include "base/allocator/partition_alloc_features.h"
#include "base/allocator/partition_allocator/src/partition_alloc/partition_alloc.h"
#include "base/bits.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/task/current_thread.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool/initialization_util.h"
#include "gin/array_buffer.h"
#include "gin/v8_initializer.h"
#include "shell/browser/microtasks_runner.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/node_includes.h"
#include "third_party/blink/public/common/switches.h"
#include "third_party/electron_node/src/node_wasm_web_api.h"

namespace {
v8::Isolate* g_isolate;
}

namespace electron {

namespace {

gin::IsolateHolder CreateIsolateHolder(v8::Isolate* isolate,
                                       size_t* max_young_generation_size) {
  std::unique_ptr<v8::Isolate::CreateParams> create_params =
      gin::IsolateHolder::getDefaultIsolateParams();
  // The value is needed to adjust heap limit when capturing
  // snapshot via v8.setHeapSnapshotNearHeapLimit(limit) or
  // --heapsnapshot-near-heap-limit=max_count.
  *max_young_generation_size =
      create_params->constraints.max_young_generation_size_in_bytes();
  // Align behavior with V8 Isolate default for Node.js.
  // This is necessary for important aspects of Node.js
  // including heap and cpu profilers to function properly.

  return {base::SingleThreadTaskRunner::GetCurrentDefault(),
          gin::IsolateHolder::kSingleThread,
          gin::IsolateHolder::IsolateType::kUtility,
          std::move(create_params),
          gin::IsolateHolder::IsolateCreationMode::kNormal,
          nullptr,
          nullptr,
          isolate};
}

}  // namespace

JavascriptEnvironment::JavascriptEnvironment(uv_loop_t* event_loop,
                                             bool setup_wasm_streaming)
    : isolate_holder_{CreateIsolateHolder(
          Initialize(event_loop, setup_wasm_streaming),
          &max_young_generation_size_)},
      isolate_{isolate_holder_.isolate()},
      locker_{isolate_} {
  isolate_->Enter();

  v8::HandleScope scope(isolate_);
  auto context = node::NewContext(isolate_);
  CHECK(!context.IsEmpty());

  context->Enter();
}

JavascriptEnvironment::~JavascriptEnvironment() {
  DCHECK_NE(platform_, nullptr);

  {
    v8::HandleScope scope(isolate_);
    isolate_->GetCurrentContext()->Exit();
  }
  isolate_->Exit();
  g_isolate = nullptr;

  platform_->UnregisterIsolate(isolate_);
}

v8::Isolate* JavascriptEnvironment::Initialize(uv_loop_t* event_loop,
                                               bool setup_wasm_streaming) {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  // --js-flags.
  std::string js_flags = "--no-freeze-flags-after-init ";
  js_flags.append(cmd->GetSwitchValueASCII(blink::switches::kJavaScriptFlags));
  v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  // The V8Platform of gin relies on Chromium's task schedule, which has not
  // been started at this point, so we have to rely on Node's V8Platform.
  auto* tracing_agent = new node::tracing::Agent();
  auto* tracing_controller = tracing_agent->GetTracingController();
  node::tracing::TraceEventHelper::SetAgent(tracing_agent);
  platform_ = node::MultiIsolatePlatform::Create(
      base::RecommendedMaxNumberOfThreadsInThreadGroup(3, 8, 0.1, 0),
      tracing_controller, gin::V8Platform::GetCurrentPageAllocator());

  v8::V8::InitializePlatform(platform_.get());
  gin::IsolateHolder::Initialize(
      gin::IsolateHolder::kNonStrictMode,
      gin::ArrayBufferAllocator::SharedInstance(),
      nullptr /* external_reference_table */, js_flags,
      false /* disallow_v8_feature_flag_overrides */,
      nullptr /* fatal_error_callback */, nullptr /* oom_error_callback */,
      false /* create_v8_platform */);

  v8::Isolate* isolate = v8::Isolate::Allocate();
  platform_->RegisterIsolate(isolate, event_loop);

  // This is done here because V8 checks for the callback in NewContext.
  // Our setup order doesn't allow for calling SetupIsolateForNode
  // before NewContext without polluting JavaScriptEnvironment with
  // Node.js logic and so we conditionally do it here to keep
  // concerns separate.
  if (setup_wasm_streaming) {
    isolate->SetWasmStreamingCallback(
        node::wasm_web_api::StartStreamingCompilation);
  }

  g_isolate = isolate;

  return isolate;
}

// static
v8::Isolate* JavascriptEnvironment::GetIsolate() {
  CHECK(g_isolate);
  return g_isolate;
}

void JavascriptEnvironment::CreateMicrotasksRunner() {
  DCHECK(!microtasks_runner_);
  microtasks_runner_ = std::make_unique<MicrotasksRunner>(isolate());
  isolate_holder_.WillCreateMicrotasksRunner();
  base::CurrentThread::Get()->AddTaskObserver(microtasks_runner_.get());
}

void JavascriptEnvironment::DestroyMicrotasksRunner() {
  DCHECK(microtasks_runner_);
  // Should be called before running gin_helper::CleanedUpAtExit::DoCleanup.
  // This helps to signal wrappable finalizer callbacks to not act on freed
  // parameters.
  isolate_holder_.WillDestroyMicrotasksRunner();
  {
    v8::HandleScope scope(isolate_);
    gin_helper::CleanedUpAtExit::DoCleanup();
  }
  base::CurrentThread::Get()->RemoveTaskObserver(microtasks_runner_.get());
}

}  // namespace electron
