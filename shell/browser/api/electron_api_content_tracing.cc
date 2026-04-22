// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_config.h"
#if !defined(ADDRESS_SANITIZER)
#include "components/heap_profiling/multi_process/client_connection_manager.h"
#include "components/heap_profiling/multi_process/supervisor.h"
#include "components/services/heap_profiling/public/cpp/settings.h"
#endif  // !defined(ADDRESS_SANITIZER)
#include "content/public/browser/tracing_controller.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

using content::TracingController;
using namespace std::literals;

namespace gin {

template <>
struct Converter<base::trace_event::TraceConfig> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::trace_event::TraceConfig* out) {
    // (alexeykuzmin): A combination of "categoryFilter" and "traceOptions"
    // has to be checked first because none of the fields
    // in the `memory_dump_config` dict below are mandatory
    // and we cannot check the config format.
    gin_helper::Dictionary options;
    if (ConvertFromV8(isolate, val, &options)) {
      std::string category_filter, trace_options;
      if (options.Get("categoryFilter", &category_filter) &&
          options.Get("traceOptions", &trace_options)) {
        *out = base::trace_event::TraceConfig(category_filter, trace_options);
        return true;
      }
    }

    base::DictValue memory_dump_config;
    if (ConvertFromV8(isolate, val, &memory_dump_config)) {
      *out = base::trace_event::TraceConfig(std::move(memory_dump_config));
      return true;
    }

    return false;
  }
};

}  // namespace gin

namespace {

using CompletionCallback = base::OnceCallback<void(const base::FilePath&)>;

std::optional<base::FilePath> CreateTemporaryFileOnIO() {
  base::FilePath temp_file_path;
  if (!base::CreateTemporaryFile(&temp_file_path))
    return std::nullopt;
  return std::make_optional(std::move(temp_file_path));
}

void StopTracing(gin_helper::Promise<base::FilePath> promise,
                 std::optional<base::FilePath> file_path) {
  auto resolve_or_reject = base::BindOnce(
      [](gin_helper::Promise<base::FilePath> promise,
         const base::FilePath& path, const std::string_view error) {
        if (!std::empty(error)) {
          promise.RejectWithErrorMessage(error);
        } else {
          promise.Resolve(path);
        }
      },
      std::move(promise), *file_path);

  auto* instance = TracingController::GetInstance();
  if (!instance->IsTracing()) {
    std::move(resolve_or_reject)
        .Run("Failed to stop tracing - no trace in progress"sv);
  } else if (file_path) {
    auto split_callback = base::SplitOnceCallback(std::move(resolve_or_reject));
    auto endpoint = TracingController::CreateFileEndpoint(
        *file_path, base::BindOnce(std::move(split_callback.first), ""sv));
    if (!instance->StopTracing(endpoint)) {
      std::move(split_callback.second).Run("Failed to stop tracing"sv);
    }
  } else {
    std::move(resolve_or_reject)
        .Run("Failed to create temporary file for trace data"sv);
  }
}

v8::Local<v8::Promise> StopRecording(gin::Arguments* const args) {
  gin_helper::Promise<base::FilePath> promise{args->isolate()};
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!electron::Browser::Get()->is_ready()) {
    promise.RejectWithErrorMessage(
        "contentTracing cannot be used before app is ready");
    return handle;
  }

  base::FilePath path;
  if (args->GetNext(&path) && !path.empty()) {
    StopTracing(std::move(promise), std::make_optional(path));
  } else {
    // use a temporary file.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(CreateTemporaryFileOnIO),
        base::BindOnce(StopTracing, std::move(promise)));
  }

  return handle;
}

v8::Local<v8::Promise> GetCategories(v8::Isolate* isolate) {
  gin_helper::Promise<const std::set<std::string>&> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!electron::Browser::Get()->is_ready()) {
    promise.RejectWithErrorMessage(
        "contentTracing cannot be used before app is ready");
    return handle;
  }

  // Note: This method always succeeds.
  TracingController::GetInstance()->GetCategories(base::BindOnce(
      gin_helper::Promise<const std::set<std::string>&>::ResolvePromise,
      std::move(promise)));

  return handle;
}

#if !defined(ADDRESS_SANITIZER)

std::tuple<heap_profiling::Mode, heap_profiling::mojom::StackMode, uint32_t>
GetHeapProfilingOptions(gin::Arguments* const args) {
  heap_profiling::Mode mode = heap_profiling::Mode::kAll;
  heap_profiling::mojom::StackMode stack_mode =
      heap_profiling::mojom::StackMode::NATIVE_WITHOUT_THREAD_NAMES;
  uint32_t sampling_rate = 100000;

  gin_helper::Dictionary options;

  if (args->GetNext(&options)) {
    std::string mode_in;
    std::string stack_mode_in;
    std::optional<uint32_t> sampling_rate_in;

    if (options.Get("mode", &mode_in)) {
      heap_profiling::Mode converted =
          heap_profiling::ConvertStringToMode(mode_in);
      if (converted != heap_profiling::Mode::kNone &&
          converted != heap_profiling::Mode::kManual) {
        mode = converted;
      }
    }
    if (options.Get("stackMode", &stack_mode_in)) {
      stack_mode = heap_profiling::ConvertStringToStackMode(stack_mode_in);
    }
    if (options.GetOptional("samplingRate", &sampling_rate_in) &&
        sampling_rate_in && sampling_rate_in.value() >= 1000 &&
        sampling_rate_in.value() <= 10000000) {
      sampling_rate = sampling_rate_in.value();
    }
  }

  return {mode, stack_mode, sampling_rate};
}

bool g_heap_profiling_started = false;
bool g_heap_profiling_stopping = false;

#endif  // !defined(ADDRESS_SANITIZER)

v8::Local<v8::Promise> EnableHeapProfiling(gin::Arguments* const args) {
#if defined(ADDRESS_SANITIZER)
  // Memory sanitizers are using large memory shadow to keep track of memory
  // state. Using memlog and memory sanitizers at the same time is slowing down
  // user experience, causing the browser to be barely responsive. In theory,
  // memlog and memory sanitizers are compatible and can run at the same time.
  return gin_helper::Promise<void>::ResolvedPromise(args->isolate());
#else
  gin_helper::Promise<void> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* supervisor = heap_profiling::Supervisor::GetInstance();

  if (g_heap_profiling_stopping) {
    promise.RejectWithErrorMessage(
        "Heap profiling is currently being disabled");
    return handle;
  }

  if (supervisor->HasStarted() || g_heap_profiling_started) {
    promise.RejectWithErrorMessage("Heap profiling is already enabled");
    return handle;
  }

  // HasStarted() becomes true asynchronously. We keep track of whether we have
  // called Start() already to avoid calling Start() twice.
  g_heap_profiling_started = true;

  auto [mode, stack_mode, sampling_rate] = GetHeapProfilingOptions(args);

  supervisor->SetClientConnectionManagerConstructor(
      [](base::WeakPtr<heap_profiling::Controller> controller_weak_ptr,
         heap_profiling::Mode mode) {
        return std::make_unique<heap_profiling::ClientConnectionManager>(
            controller_weak_ptr, mode);
      });

  supervisor->Start(mode, stack_mode, sampling_rate,
                    base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                                   std::move(promise)));

  return handle;
#endif  // defined(ADDRESS_SANITIZER)
}

v8::Local<v8::Promise> DisableHeapProfiling(gin::Arguments* const args) {
#if defined(ADDRESS_SANITIZER)
  return gin_helper::Promise<void>::ResolvedPromise(args->isolate());
#else
  gin_helper::Promise<void> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* supervisor = heap_profiling::Supervisor::GetInstance();

  if (g_heap_profiling_stopping) {
    promise.RejectWithErrorMessage(
        "Heap profiling is currently being disabled");
    return handle;
  }

  if (!supervisor->HasStarted() && !g_heap_profiling_started) {
    promise.RejectWithErrorMessage("Heap profiling is not enabled");
    return handle;
  }

  if (!supervisor->HasStarted() && g_heap_profiling_started) {
    promise.RejectWithErrorMessage("Heap profiling is currently being enabled");
    return handle;
  }

  g_heap_profiling_stopping = true;
  supervisor->Stop(base::BindOnce(
      [](gin_helper::Promise<void> promise, bool success) {
        g_heap_profiling_stopping = false;
        if (!success) {
          gin_helper::Promise<void>::RejectPromise(
              std::move(promise), "Failed to disable heap profiling");
          return;
        }

        g_heap_profiling_started = false;
        gin_helper::Promise<void>::ResolvePromise(std::move(promise));
      },
      std::move(promise)));

  return handle;
#endif
}

v8::Local<v8::Promise> StartTracing(
    v8::Isolate* isolate,
    const base::trace_event::TraceConfig& trace_config) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!electron::Browser::Get()->is_ready()) {
    promise.RejectWithErrorMessage(
        "contentTracing cannot be used before app is ready");
    return handle;
  }

  if (!TracingController::GetInstance()->StartTracing(
          trace_config,
          base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                         std::move(promise)))) {
    // If StartTracing returns false, that means it didn't invoke its callback.
    // Return an already-resolved promise and abandon the previous promise (it
    // was std::move()d into the StartTracing callback and has been deleted by
    // this point).
    return gin_helper::Promise<void>::ResolvedPromise(isolate);
  }
  return handle;
}

void OnTraceBufferUsageAvailable(
    gin_helper::Promise<gin_helper::Dictionary> promise,
    float percent_full,
    size_t approximate_count) {
  v8::Isolate* isolate = promise.isolate();
  v8::HandleScope handle_scope(isolate);

  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("percentage", percent_full);
  dict.Set("value", approximate_count);

  promise.Resolve(dict);
}

v8::Local<v8::Promise> GetTraceBufferUsage(v8::Isolate* isolate) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!electron::Browser::Get()->is_ready()) {
    promise.RejectWithErrorMessage(
        "contentTracing cannot be used before app is ready");
    return handle;
  }

  // Note: This method always succeeds.
  TracingController::GetInstance()->GetTraceBufferUsage(
      base::BindOnce(&OnTraceBufferUsageAvailable, std::move(promise)));
  return handle;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("getCategories", &GetCategories);
  dict.SetMethod("startRecording", &StartTracing);
  dict.SetMethod("stopRecording", &StopRecording);
  dict.SetMethod("getTraceBufferUsage", &GetTraceBufferUsage);
  dict.SetMethod("enableHeapProfiling", &EnableHeapProfiling);
  dict.SetMethod("disableHeapProfiling", &DisableHeapProfiling);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_content_tracing, Initialize)
