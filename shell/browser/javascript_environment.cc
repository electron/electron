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
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "shell/browser/microtasks_runner.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/node_includes.h"
#include "shell/common/process_util.h"
#include "third_party/blink/public/common/switches.h"
#include "third_party/electron_node/src/node_snapshot_builder.h"
#include "third_party/electron_node/src/node_wasm_web_api.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-locker.h"

namespace {
v8::Isolate* g_isolate;
}

namespace electron {

namespace {

// The Node startup snapshot a JavascriptEnvironment's isolate is created
// from, when one is embedded and this process is allowed to consume it.
// The snapshot was built by extending the same snapshot_blob.bin the rest
// of the process's V8 uses, so the read-only heap is shared (V8's
// shared-RO-heap requirement). Browser-process-only: the renderer's isolate
// must use the blink v8_context_snapshot, and the utility-process node
// service does its own thing.
const node::SnapshotData* NodeSnapshotForThisProcess() {
  if (!electron::IsBrowserProcess())
    return nullptr;
  // The ELECTRON_RUN_AS_NODE entry point (shell/app/node_main.cc) runs without
  // a process type, so IsBrowserProcess() is true for it too -- but it boots
  // Node directly and builds its IsolateData without snapshot_data, so handing
  // it a snapshot-backed isolate would trip node::CreateEnvironment's
  // snapshot_data CHECK. It isn't the app-start path this targets; skip it.
  if (electron::IsRunningAsNode())
    return nullptr;
  return node::SnapshotBuilder::GetEmbeddedSnapshotData();
}

std::unique_ptr<gin::IsolateHolder> CreateIsolateHolder(
    v8::Isolate* isolate,
    size_t* max_young_generation_size) {
  std::unique_ptr<v8::Isolate::CreateParams> create_params =
      gin::IsolateHolder::getDefaultIsolateParams();
  // The value is needed to adjust heap limit when capturing
  // snapshot via v8.setHeapSnapshotNearHeapLimit(limit) or
  // --heapsnapshot-near-heap-limit=max_count.
  *max_young_generation_size =
      create_params->constraints.max_young_generation_size_in_bytes();
  // Electron: create the browser-process isolate from the embedded Node
  // startup snapshot when one is present, so the Node bootstrap is
  // deserialized instead of recompiled at app start.
  if (const node::SnapshotData* sd = NodeSnapshotForThisProcess()) {
    node::SnapshotBuilder::InitializeIsolateParams(sd, create_params.get());
  }
  // Align behavior with V8 Isolate default for Node.js.
  // This is necessary for important aspects of Node.js
  // including heap and cpu profilers to function properly.

  return std::make_unique<gin::IsolateHolder>(
      base::SingleThreadTaskRunner::GetCurrentDefault(),
      gin::IsolateHolder::kSingleThread,
      gin::IsolateHolder::IsolateType::kUtility, std::move(create_params),
      gin::IsolateHolder::IsolateCreationMode::kNormal, nullptr, nullptr,
      isolate);
}

}  // namespace

JavascriptEnvironment::JavascriptEnvironment(uv_loop_t* event_loop,
                                             bool setup_wasm_streaming)
    : isolate_holder_{
          CreateIsolateHolder(Initialize(event_loop, setup_wasm_streaming),
                              &max_young_generation_size_)},
      locker_{std::in_place, isolate()} {
  v8::Isolate* const isolate = this->isolate();
  isolate->Enter();

  // Electron: when consuming the embedded Node startup snapshot, the main
  // context is materialized from the snapshot inside node::CreateEnvironment
  // (Context::FromSnapshot) and entered in electron_browser_main_parts.cc
  // after that. Creating a fresh node::NewContext here would be wasted work.
  if (NodeSnapshotForThisProcess() != nullptr)
    return;

  v8::HandleScope scope{isolate};
  auto context = node::NewContext(isolate);
  CHECK(!context.IsEmpty());

  context->Enter();
}

JavascriptEnvironment::~JavascriptEnvironment() {
  DCHECK_NE(platform_, nullptr);
  v8::Isolate* isolate = this->isolate();

  {
    v8::HandleScope scope{isolate};
    isolate->GetCurrentContext()->Exit();
  }
  isolate->Exit();
  g_isolate = nullptr;

  // Deinit gin::IsolateHolder prior to calling NodePlatform::UnregisterIsolate.
  // Otherwise cppgc::internal::Sweeper::Start will try to request a task runner
  // from the NodePlatform with an already unregistered isolate.
  locker_.reset();
  DCHECK(!microtasks_runner_);
  isolate_holder_.reset();

  platform_->UnregisterIsolate(isolate);
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
      tracing_controller, gin::V8Platform::Get()->GetPageAllocator());

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

v8::Isolate* JavascriptEnvironment::isolate() const {
  return isolate_holder_->isolate();
}

// static
v8::Isolate* JavascriptEnvironment::GetIsolate() {
  CHECK(g_isolate);
  return g_isolate;
}

void JavascriptEnvironment::CreateMicrotasksRunner() {
  DCHECK(!microtasks_runner_);
  microtasks_runner_ = std::make_unique<MicrotasksRunner>(isolate());
  isolate_holder_->WillCreateMicrotasksRunner();
  base::CurrentThread::Get()->AddTaskObserver(microtasks_runner_.get());
}

void JavascriptEnvironment::DestroyMicrotasksRunner() {
  DCHECK(microtasks_runner_);
  // Should be called before running gin_helper::CleanedUpAtExit::DoCleanup.
  // This helps to signal wrappable finalizer callbacks to not act on freed
  // parameters.
  isolate_holder_->WillDestroyMicrotasksRunner();
  {
    v8::HandleScope scope{isolate()};
    gin_helper::CleanedUpAtExit::DoCleanup();
  }
  base::CurrentThread::Get()->RemoveTaskObserver(microtasks_runner_.get());
  microtasks_runner_.reset();
}

}  // namespace electron
