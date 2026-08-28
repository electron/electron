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
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/task/current_thread.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool/initialization_util.h"
#include "electron/snapshot_checksum.h"
#include "gin/array_buffer.h"
#include "gin/per_isolate_data.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "shell/browser/microtasks_runner.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/process_util.h"
#include "third_party/blink/public/common/switches.h"
#include "third_party/electron_node/src/node_snapshot_builder.h"
#include "third_party/electron_node/src/node_wasm_web_api.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-locker.h"
#include "v8/include/v8-snapshot.h"

namespace {
v8::Isolate* g_isolate;
}

namespace electron {

namespace {

// Whether the V8 snapshot blob this process loaded (v8_context_snapshot.bin,
// or in the browser process under the LoadBrowserProcessSpecificV8Snapshot
// fuse, browser_v8_context_snapshot.bin) is the one this build shipped, as
// opposed to a custom blob produced with electron-mksnapshot. Compares size
// and V8's blob header -- which embeds V8's checksum of the rest of the
// blob -- against build-time constants, so no hashing at startup and no
// per-process-type knowledge of which file was picked.
bool LoadedV8SnapshotIsBuiltIn() {
  v8::StartupData blob{nullptr, 0};
  gin::V8Initializer::GetV8ExternalSnapshotData(&blob);
  if (!blob.data || blob.raw_size <= 0)
    return false;
  const auto loaded = base::as_bytes(UNSAFE_BUFFERS(
      base::span<const char>(blob.data, static_cast<size_t>(blob.raw_size))));
  const auto expected_prefix = base::span(snapshot_checksum::kHeaderPrefix);
  return loaded.size() == snapshot_checksum::kSize &&
         loaded.size() >= expected_prefix.size() &&
         loaded.first(expected_prefix.size()) == expected_prefix;
}

// The Node startup snapshot a JavascriptEnvironment's isolate is created
// from, when one is embedded and this process is allowed to consume it: the
// browser process, ELECTRON_RUN_AS_NODE children (shell/app/node_main.cc,
// which also run without a --type) and the utility process hosting the node
// service (the only utility-process code that creates a JavascriptEnvironment).
// Each creates exactly one main isolate -- Node worker threads inherit the
// snapshot through IsolateData -- so V8's one-read-only-heap-per-process rule
// holds even though the process-wide external startup data content/gin loaded
// is the v8 context snapshot. (Renderers never have a JavascriptEnvironment;
// their isolates belong to blink.)
const node::SnapshotData* NodeSnapshotForThisProcess() {
  static const node::SnapshotData* const snapshot =
      []() -> const node::SnapshotData* {
    if (!electron::IsBrowserProcess() && !electron::IsUtilityProcess())
      return nullptr;
    const node::SnapshotData* embedded =
        node::SnapshotBuilder::GetEmbeddedSnapshotData();
    if (!embedded)
      return nullptr;
    // A custom V8 snapshot (electron-mksnapshot, or the browser-specific blob
    // the LoadBrowserProcessSpecificV8Snapshot fuse selects) exists to put
    // objects into the main process's context. The embedded Node snapshot was
    // built on top of the stock blob at build time and cannot contain them, so
    // creating the main context from it would silently drop the custom
    // snapshot's contents. Fall back to bootstrapping Node from scratch on the
    // loaded blob, as builds without a Node snapshot do.
    if (!LoadedV8SnapshotIsBuiltIn()) {
      VLOG(1) << "Custom V8 snapshot loaded; not creating this process's "
                 "Node.js environment from the embedded Node snapshot.";
      return nullptr;
    }
    return embedded;
  }();
  return snapshot;
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

JavascriptEnvironment::JavascriptEnvironment(
    uv_loop_t* event_loop,
    bool setup_wasm_streaming,
    v8::TracingController* tracing_controller)
    : isolate_holder_{CreateIsolateHolder(
          Initialize(event_loop, setup_wasm_streaming, tracing_controller),
          &max_young_generation_size_)},
      locker_{std::make_unique<v8::Locker>(isolate())} {
  v8::Isolate* const isolate = this->isolate();
  isolate->Enter();

  // Every JavascriptEnvironment hosts a Node.js environment (browser process,
  // ELECTRON_RUN_AS_NODE, the utility-process node service). Install the
  // build-time builtin code cache before node::NewContext below constructs
  // the first BuiltinLoader (for the per-context scripts).
  electron::util::InstallProcessCodeCache();

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

v8::Isolate* JavascriptEnvironment::Initialize(
    uv_loop_t* event_loop,
    bool setup_wasm_streaming,
    v8::TracingController* tracing_controller) {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  // --js-flags.
  std::string js_flags = "--no-freeze-flags-after-init ";
  js_flags.append(cmd->GetSwitchValueASCII(blink::switches::kJavaScriptFlags));
  v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  // The V8Platform of gin relies on Chromium's task schedule, which has not
  // been started at this point, so we have to rely on Node's V8Platform.
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

// static
const node::SnapshotData* JavascriptEnvironment::NodeSnapshot() {
  return NodeSnapshotForThisProcess();
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
  // After DoCleanup() so that observers created by JS that ran during it (e.g.
  // a webContents 'destroyed' handler) are notified too.
  gin::PerIsolateData::From(isolate())->NotifyBeforeMicrotasksRunnerDispose();
  base::CurrentThread::Get()->RemoveTaskObserver(microtasks_runner_.get());
  microtasks_runner_.reset();
}

}  // namespace electron
