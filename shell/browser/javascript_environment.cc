// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/javascript_environment.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/allocator/partition_alloc_features.h"
#include "base/allocator/partition_allocator/src/partition_alloc/partition_alloc.h"
#include "base/bits.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/task/current_thread.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool/initialization_util.h"
#include "base/trace_event/trace_event.h"
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

namespace gin {

class ConvertableToTraceFormatWrapper final
    : public base::trace_event::ConvertableToTraceFormat {
 public:
  explicit ConvertableToTraceFormatWrapper(
      std::unique_ptr<v8::ConvertableToTraceFormat> inner)
      : inner_(std::move(inner)) {}
  ~ConvertableToTraceFormatWrapper() override = default;

  // disable copy
  ConvertableToTraceFormatWrapper(const ConvertableToTraceFormatWrapper&) =
      delete;
  ConvertableToTraceFormatWrapper& operator=(
      const ConvertableToTraceFormatWrapper&) = delete;

  void AppendAsTraceFormat(std::string* out) const final {
    inner_->AppendAsTraceFormat(out);
  }

 private:
  std::unique_ptr<v8::ConvertableToTraceFormat> inner_;
};

}  // namespace gin

// Allow std::unique_ptr<v8::ConvertableToTraceFormat> to be a valid
// initialization value for trace macros.
template <>
struct base::trace_event::TraceValue::Helper<
    std::unique_ptr<v8::ConvertableToTraceFormat>> {
  static constexpr unsigned char kType = TRACE_VALUE_TYPE_CONVERTABLE;
  static inline void SetValue(
      TraceValue* v,
      std::unique_ptr<v8::ConvertableToTraceFormat> value) {
    // NOTE: |as_convertable| is an owning pointer, so using new here
    // is acceptable.
    v->as_convertable =
        new gin::ConvertableToTraceFormatWrapper(std::move(value));
  }
};

namespace electron {

namespace {

gin::IsolateHolder CreateIsolateHolder(v8::Isolate* isolate) {
  std::unique_ptr<v8::Isolate::CreateParams> create_params =
      gin::IsolateHolder::getDefaultIsolateParams();
  // Align behavior with V8 Isolate default for Node.js.
  // This is necessary for important aspects of Node.js
  // including heap and cpu profilers to function properly.
  //
  // Additional note:
  // We do not want to invoke a termination exception at exit when
  // we're running with only_terminate_in_safe_scope set to false. Heap and
  // coverage profilers run after environment exit and if there is a pending
  // exception at this stage then they will fail to generate the appropriate
  // profiles. Node.js does not call node::Stop(), which calls
  // isolate->TerminateExecution(), and therefore does not have this issue
  // when also running with only_terminate_in_safe_scope set to false.
  create_params->only_terminate_in_safe_scope = false;

  return gin::IsolateHolder(
      base::SingleThreadTaskRunner::GetCurrentDefault(),
      gin::IsolateHolder::kSingleThread,
      gin::IsolateHolder::IsolateType::kUtility, std::move(create_params),
      gin::IsolateHolder::IsolateCreationMode::kNormal, nullptr, isolate);
}

}  // namespace

JavascriptEnvironment::JavascriptEnvironment(uv_loop_t* event_loop,
                                             bool setup_wasm_streaming)
    : isolate_holder_{CreateIsolateHolder(
          Initialize(event_loop, setup_wasm_streaming))},
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
  platform_->DrainTasks(isolate_);

  {
    v8::HandleScope scope(isolate_);
    isolate_->GetCurrentContext()->Exit();
  }
  isolate_->Exit();
  g_isolate = nullptr;

  platform_->UnregisterIsolate(isolate_);
}

class EnabledStateObserverImpl final
    : public base::trace_event::TraceLog::EnabledStateObserver {
 public:
  EnabledStateObserverImpl() {
    base::trace_event::TraceLog::GetInstance()->AddEnabledStateObserver(this);
  }

  ~EnabledStateObserverImpl() override {
    base::trace_event::TraceLog::GetInstance()->RemoveEnabledStateObserver(
        this);
  }

  // disable copy
  EnabledStateObserverImpl(const EnabledStateObserverImpl&) = delete;
  EnabledStateObserverImpl& operator=(const EnabledStateObserverImpl&) = delete;

  void OnTraceLogEnabled() final {
    base::AutoLock lock(mutex_);
    for (auto* o : observers_) {
      o->OnTraceEnabled();
    }
  }

  void OnTraceLogDisabled() final {
    base::AutoLock lock(mutex_);
    for (auto* o : observers_) {
      o->OnTraceDisabled();
    }
  }

  void AddObserver(v8::TracingController::TraceStateObserver* observer) {
    {
      base::AutoLock lock(mutex_);
      DCHECK(!observers_.count(observer));
      observers_.insert(observer);
    }

    // Fire the observer if recording is already in progress.
    if (base::trace_event::TraceLog::GetInstance()->IsEnabled())
      observer->OnTraceEnabled();
  }

  void RemoveObserver(v8::TracingController::TraceStateObserver* observer) {
    base::AutoLock lock(mutex_);
    DCHECK_EQ(observers_.count(observer), 1lu);
    observers_.erase(observer);
  }

 private:
  base::Lock mutex_;
  std::unordered_set<v8::TracingController::TraceStateObserver*> observers_;
};

base::LazyInstance<EnabledStateObserverImpl>::Leaky g_trace_state_dispatcher =
    LAZY_INSTANCE_INITIALIZER;

class TracingControllerImpl : public node::tracing::TracingController {
 public:
  TracingControllerImpl() = default;
  ~TracingControllerImpl() override = default;

  // disable copy
  TracingControllerImpl(const TracingControllerImpl&) = delete;
  TracingControllerImpl& operator=(const TracingControllerImpl&) = delete;

  // TracingController implementation.
  const uint8_t* GetCategoryGroupEnabled(const char* name) override {
    return TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(name);
  }
  uint64_t AddTraceEvent(
      char phase,
      const uint8_t* category_enabled_flag,
      const char* name,
      const char* scope,
      uint64_t id,
      uint64_t bind_id,
      int32_t num_args,
      const char** arg_names,
      const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags) override {
    base::trace_event::TraceArguments args(
        num_args, arg_names, arg_types,
        reinterpret_cast<const unsigned long long*>(  // NOLINT(runtime/int)
            arg_values),
        arg_convertables);
    DCHECK_LE(num_args, 2);
    base::trace_event::TraceEventHandle handle =
        TRACE_EVENT_API_ADD_TRACE_EVENT_WITH_BIND_ID(
            phase, category_enabled_flag, name, scope, id, bind_id, &args,
            flags);
    uint64_t result;
    memcpy(&result, &handle, sizeof(result));
    return result;
  }
  uint64_t AddTraceEventWithTimestamp(
      char phase,
      const uint8_t* category_enabled_flag,
      const char* name,
      const char* scope,
      uint64_t id,
      uint64_t bind_id,
      int32_t num_args,
      const char** arg_names,
      const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags,
      int64_t timestampMicroseconds) override {
    base::trace_event::TraceArguments args(
        num_args, arg_names, arg_types,
        reinterpret_cast<const unsigned long long*>(  // NOLINT(runtime/int)
            arg_values),
        arg_convertables);
    DCHECK_LE(num_args, 2);
    base::TimeTicks timestamp =
        base::TimeTicks() + base::Microseconds(timestampMicroseconds);
    base::trace_event::TraceEventHandle handle =
        TRACE_EVENT_API_ADD_TRACE_EVENT_WITH_THREAD_ID_AND_TIMESTAMP(
            phase, category_enabled_flag, name, scope, id, bind_id,
            TRACE_EVENT_API_CURRENT_THREAD_ID, timestamp, &args, flags);
    uint64_t result;
    memcpy(&result, &handle, sizeof(result));
    return result;
  }
  void UpdateTraceEventDuration(const uint8_t* category_enabled_flag,
                                const char* name,
                                uint64_t handle) override {
    base::trace_event::TraceEventHandle traceEventHandle;
    memcpy(&traceEventHandle, &handle, sizeof(handle));
    TRACE_EVENT_API_UPDATE_TRACE_EVENT_DURATION(category_enabled_flag, name,
                                                traceEventHandle);
  }
  void AddTraceStateObserver(TraceStateObserver* observer) override {
    g_trace_state_dispatcher.Get().AddObserver(observer);
  }
  void RemoveTraceStateObserver(TraceStateObserver* observer) override {
    g_trace_state_dispatcher.Get().RemoveObserver(observer);
  }
};

v8::Isolate* JavascriptEnvironment::Initialize(uv_loop_t* event_loop,
                                               bool setup_wasm_streaming) {
  auto* cmd = base::CommandLine::ForCurrentProcess();

  // --js-flags.
  std::string js_flags =
      cmd->GetSwitchValueASCII(blink::switches::kJavaScriptFlags);
  js_flags.append(" --no-freeze-flags-after-init");
  if (!js_flags.empty())
    v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  // The V8Platform of gin relies on Chromium's task schedule, which has not
  // been started at this point, so we have to rely on Node's V8Platform.
  auto* tracing_agent = node::CreateAgent();
  auto* tracing_controller = new TracingControllerImpl();
  node::tracing::TraceEventHelper::SetAgent(tracing_agent);
  platform_ = node::MultiIsolatePlatform::Create(
      base::RecommendedMaxNumberOfThreadsInThreadGroup(3, 8, 0.1, 0),
      tracing_controller, gin::V8Platform::GetCurrentPageAllocator());

  v8::V8::InitializePlatform(platform_.get());
  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
                                 gin::ArrayBufferAllocator::SharedInstance(),
                                 nullptr /* external_reference_table */,
                                 js_flags, nullptr /* fatal_error_callback */,
                                 nullptr /* oom_error_callback */,
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
  base::CurrentThread::Get()->AddTaskObserver(microtasks_runner_.get());
}

void JavascriptEnvironment::DestroyMicrotasksRunner() {
  DCHECK(microtasks_runner_);
  {
    v8::HandleScope scope(isolate_);
    gin_helper::CleanedUpAtExit::DoCleanup();
  }
  base::CurrentThread::Get()->RemoveTaskObserver(microtasks_runner_.get());
}

}  // namespace electron
