// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_config.h"
#include "content/public/browser/tracing_controller.h"
#include "services/tracing/public/cpp/perfetto/perfetto_config.h"
#include "services/tracing/public/cpp/perfetto/perfetto_data_source_names.h"
#include "services/tracing/public/cpp/perfetto/perfetto_session.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "third_party/perfetto/include/perfetto/tracing/tracing.h"
#include "third_party/perfetto/protos/perfetto/config/chrome/sampling_heap_profiler.gen.h"

using content::TracingController;
using namespace std::literals;

namespace {

struct HeapProfilerOptions {
  uint64_t sampling_interval_bytes = 128 * 1024;
  uint32_t sampling_interval_ms = 50;
};

struct ContentTracingConfig {
  base::trace_event::TraceConfig trace_config;
  std::optional<HeapProfilerOptions> heap_profiler_options;
};

template <typename T>
bool GetOptionalUnsignedInteger(const gin_helper::Dictionary& options,
                                std::string_view key,
                                T* out) {
  constexpr double kMaxSafeInteger = 9007199254740991.0;
  if (!options.Has(key))
    return true;

  double value;
  if (!options.Get(key, &value) || !std::isfinite(value) || value < 0 ||
      std::floor(value) != value || value > kMaxSafeInteger ||
      value > static_cast<double>(std::numeric_limits<T>::max())) {
    return false;
  }
  *out = static_cast<T>(value);
  return true;
}

}  // namespace

namespace gin {

template <>
struct Converter<HeapProfilerOptions> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     HeapProfilerOptions* out) {
    gin_helper::Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;

    if (!GetOptionalUnsignedInteger(options, "sampling_interval_bytes",
                                    &out->sampling_interval_bytes) ||
        out->sampling_interval_bytes == 0) {
      return false;
    }
    return GetOptionalUnsignedInteger(options, "sampling_interval_ms",
                                      &out->sampling_interval_ms);
  }
};

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

template <>
struct Converter<ContentTracingConfig> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ContentTracingConfig* out) {
    if (!ConvertFromV8(isolate, val, &out->trace_config))
      return false;

    gin_helper::Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;

    if (!options.Has("heap_profiler_options"))
      return true;

    HeapProfilerOptions heap_profiler_options;
    if (!options.Get("heap_profiler_options", &heap_profiler_options))
      return false;
    out->heap_profiler_options = heap_profiler_options;
    return true;
  }
};

}  // namespace gin

namespace {

using StopTracingCallback = base::OnceCallback<void(const std::string_view)>;

void WriteHeapTraceToFile(const base::FilePath& file_path,
                          StopTracingCallback callback,
                          std::unique_ptr<std::string> trace) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(
          [](const base::FilePath& file_path,
             std::unique_ptr<std::string> trace) {
            return base::WriteFile(file_path, *trace);
          },
          file_path, std::move(trace)),
      base::BindOnce(
          [](StopTracingCallback callback, bool success) {
            std::move(callback).Run(success ? ""sv
                                            : "Failed to write trace data"sv);
          },
          std::move(callback)));
}

class HeapProfilingTraceSession {
 public:
  using StartCallback = base::OnceCallback<void(const std::string&)>;
  using StopCallback = base::OnceCallback<void(const std::string_view)>;

  HeapProfilingTraceSession() = default;
  HeapProfilingTraceSession(const HeapProfilingTraceSession&) = delete;
  HeapProfilingTraceSession& operator=(const HeapProfilingTraceSession&) =
      delete;

  bool IsTracing() const { return tracing_session_ != nullptr; }
  bool IsStopping() const { return !!stop_callback_; }

  void Start(const base::trace_event::TraceConfig& trace_config,
             const HeapProfilerOptions& heap_profiler_options,
             StartCallback callback) {
    weak_factory_.InvalidateWeakPtrs();
    task_runner_ = base::SequencedTaskRunner::GetCurrentDefault();
    start_callback_ = std::move(callback);
    tracing_session_ =
        perfetto::Tracing::NewTrace(perfetto::BackendType::kCustomBackend);

    perfetto::TraceConfig perfetto_config = tracing::GetDefaultPerfettoConfig(
        trace_config, /*privacy_filtering_enabled=*/false,
        /*convert_to_legacy_json=*/false);
    auto* heap_data_source = perfetto_config.add_data_sources();
    auto* data_source = heap_data_source->mutable_config();
    data_source->set_name(tracing::kNativeHeapProfilerSourceName);
    data_source->set_target_buffer(0);
    for (const auto& included_pid :
         trace_config.process_filter_config().included_process_ids()) {
      *heap_data_source->add_producer_name_filter() = base::StrCat(
          {tracing::kPerfettoProducerNamePrefix,
           base::NumberToString(static_cast<uint32_t>(included_pid))});
    }

    perfetto::protos::gen::ChromiumSamplingHeapProfilerConfig heap_config;
    heap_config.set_sampling_interval_bytes(
        heap_profiler_options.sampling_interval_bytes);
    heap_config.set_sampling_interval_ms(
        heap_profiler_options.sampling_interval_ms);
    data_source->set_chromium_sampling_heap_profiler_raw(
        heap_config.SerializeAsString());

    tracing_session_->Setup(perfetto_config);
    tracing_session_->SetOnStartCallback(
        [task_runner = task_runner_, weak_ptr = weak_factory_.GetWeakPtr()]() {
          task_runner->PostTask(
              FROM_HERE,
              base::BindOnce(&HeapProfilingTraceSession::OnTracingStart,
                             weak_ptr));
        });
    tracing_session_->SetOnStopCallback(
        [task_runner = task_runner_, weak_ptr = weak_factory_.GetWeakPtr()]() {
          task_runner->PostTask(
              FROM_HERE,
              base::BindOnce(&HeapProfilingTraceSession::OnTracingStop,
                             weak_ptr));
        });
    tracing_session_->SetOnErrorCallback(
        [task_runner = task_runner_,
         weak_ptr = weak_factory_.GetWeakPtr()](perfetto::TracingError error) {
          task_runner->PostTask(
              FROM_HERE,
              base::BindOnce(&HeapProfilingTraceSession::OnTracingError,
                             weak_ptr, std::move(error.message)));
        });
    tracing_session_->Start();
  }

  void Stop(scoped_refptr<TracingController::TraceDataEndpoint> endpoint,
            StopCallback callback) {
    trace_data_endpoint_ = std::move(endpoint);
    stop_callback_ = std::move(callback);
    tracing_session_->Stop();
  }

  void GetBufferUsage(TracingController::GetTraceBufferUsageCallback callback) {
    buffer_usage_callbacks_.push_back(std::move(callback));
    if (buffer_usage_request_pending_)
      return;

    buffer_usage_request_pending_ = true;
    tracing_session_->GetTraceStats(
        [task_runner = task_runner_, weak_ptr = weak_factory_.GetWeakPtr()](
            perfetto::TracingSession::GetTraceStatsCallbackArgs args) {
          tracing::ReadTraceStats(
              args,
              base::BindOnce(&HeapProfilingTraceSession::OnBufferUsage,
                             weak_ptr),
              task_runner);
        });
  }

 private:
  void OnTracingStart() {
    if (start_callback_)
      std::move(start_callback_).Run({});
  }

  void OnTracingError(std::string error) {
    if (start_callback_)
      std::move(start_callback_).Run(error);
    if (stop_callback_)
      std::move(stop_callback_).Run(error);
    for (auto& callback : buffer_usage_callbacks_)
      std::move(callback).Run(0, 0);
    buffer_usage_callbacks_.clear();
    buffer_usage_request_pending_ = false;
    weak_factory_.InvalidateWeakPtrs();
    tracing_session_.reset();
    trace_data_endpoint_.reset();
  }

  void OnBufferUsage(bool, float percent_full, bool) {
    buffer_usage_request_pending_ = false;
    auto callbacks = std::move(buffer_usage_callbacks_);
    buffer_usage_callbacks_.clear();
    for (auto& callback : callbacks)
      std::move(callback).Run(percent_full, 0);
  }

  void DrainBufferUsageCallbacks() {
    buffer_usage_request_pending_ = false;
    auto callbacks = std::move(buffer_usage_callbacks_);
    buffer_usage_callbacks_.clear();
    for (auto& callback : callbacks)
      std::move(callback).Run(0, 0);
  }

  void OnTracingStop() {
    if (!trace_data_endpoint_) {
      if (start_callback_) {
        std::move(start_callback_)
            .Run("Heap profiling stopped before trace data was requested");
      }
      DrainBufferUsageCallbacks();
      weak_factory_.InvalidateWeakPtrs();
      tracing_session_.reset();
      stop_callback_.Reset();
      return;
    }

    tracing_session_->ReadTrace(
        [task_runner = task_runner_, weak_ptr = weak_factory_.GetWeakPtr()](
            perfetto::TracingSession::ReadTraceCallbackArgs args) {
          auto chunk = args.size
                           ? std::make_unique<std::string>(args.data, args.size)
                           : nullptr;
          task_runner->PostTask(
              FROM_HERE,
              base::BindOnce(&HeapProfilingTraceSession::OnTraceData, weak_ptr,
                             std::move(chunk), args.has_more));
        });
  }

  void OnTraceData(std::unique_ptr<std::string> chunk, bool has_more) {
    if (chunk)
      trace_data_endpoint_->ReceiveTraceChunk(std::move(chunk));
    if (!has_more) {
      if (start_callback_)
        std::move(start_callback_).Run({});
      tracing_session_->SetOnErrorCallback({});
      DrainBufferUsageCallbacks();
      weak_factory_.InvalidateWeakPtrs();
      tracing_session_.reset();
      trace_data_endpoint_->ReceivedTraceFinalContents();
      trace_data_endpoint_.reset();
      stop_callback_.Reset();
    }
  }

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::unique_ptr<perfetto::TracingSession> tracing_session_;
  scoped_refptr<TracingController::TraceDataEndpoint> trace_data_endpoint_;
  StartCallback start_callback_;
  StopCallback stop_callback_;
  std::vector<TracingController::GetTraceBufferUsageCallback>
      buffer_usage_callbacks_;
  bool buffer_usage_request_pending_ = false;
  base::WeakPtrFactory<HeapProfilingTraceSession> weak_factory_{this};
};

HeapProfilingTraceSession& GetHeapProfilingTraceSession() {
  static base::NoDestructor<HeapProfilingTraceSession> session;
  return *session;
}

std::optional<base::FilePath> CreateTemporaryFileOnIO() {
  base::FilePath temp_file_path;
  if (!base::CreateTemporaryFile(&temp_file_path))
    return std::nullopt;
  return std::make_optional(std::move(temp_file_path));
}

void StopTracing(gin_helper::Promise<base::FilePath> promise,
                 std::optional<base::FilePath> file_path) {
  StopTracingCallback resolve_or_reject = base::BindOnce(
      [](gin_helper::Promise<base::FilePath> promise,
         const base::FilePath& path, const std::string_view error) {
        if (!std::empty(error)) {
          promise.RejectWithErrorMessage(error);
        } else {
          promise.Resolve(path);
        }
      },
      std::move(promise), file_path.value_or(base::FilePath()));

  auto* instance = TracingController::GetInstance();
  auto& heap_profiling_session = GetHeapProfilingTraceSession();
  if (!instance->IsTracing() && !heap_profiling_session.IsTracing()) {
    std::move(resolve_or_reject)
        .Run("Failed to stop tracing - no trace in progress"sv);
  } else if (heap_profiling_session.IsStopping()) {
    std::move(resolve_or_reject)
        .Run("Failed to stop tracing - trace is already stopping"sv);
  } else if (file_path) {
    auto split_callback = base::SplitOnceCallback(std::move(resolve_or_reject));
    if (heap_profiling_session.IsTracing()) {
      // Chromium's file endpoint opens files in text mode, which corrupts
      // protobuf traces on Windows. Buffer the trace first, then write it with
      // Chromium's binary-safe file API.
      auto endpoint = TracingController::CreateStringEndpoint(base::BindOnce(
          WriteHeapTraceToFile, *file_path, std::move(split_callback.first)));
      heap_profiling_session.Stop(std::move(endpoint),
                                  std::move(split_callback.second));
    } else {
      // The file endpoint hands this closure to a thread pool sequence and, if
      // it fails to write the trace file, drops it there without running it.
      // The promise it owns must be destroyed on the thread that created it,
      // so make sure both running and destroying the closure happen back on
      // this thread.
      auto endpoint = TracingController::CreateFileEndpoint(
          *file_path, base::BindPostTaskToCurrentDefault(base::BindOnce(
                          std::move(split_callback.first), ""sv)));
      if (!instance->StopTracing(endpoint)) {
        std::move(split_callback.second).Run("Failed to stop tracing"sv);
      }
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

v8::Local<v8::Promise> StartTracing(v8::Isolate* isolate,
                                    const ContentTracingConfig& config) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!electron::Browser::Get()->is_ready()) {
    promise.RejectWithErrorMessage(
        "contentTracing cannot be used before app is ready");
    return handle;
  }

  auto* instance = TracingController::GetInstance();
  auto& heap_profiling_session = GetHeapProfilingTraceSession();
  if (instance->IsTracing() || heap_profiling_session.IsTracing()) {
    return gin_helper::Promise<void>::ResolvedPromise(isolate);
  }

  if (config.heap_profiler_options) {
    heap_profiling_session.Start(
        config.trace_config, *config.heap_profiler_options,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, const std::string& error) {
              if (error.empty()) {
                promise.Resolve();
              } else {
                promise.RejectWithErrorMessage(error);
              }
            },
            std::move(promise)));
  } else if (!instance->StartTracing(
                 config.trace_config,
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
  auto& heap_profiling_session = GetHeapProfilingTraceSession();
  if (heap_profiling_session.IsTracing()) {
    heap_profiling_session.GetBufferUsage(
        base::BindOnce(&OnTraceBufferUsageAvailable, std::move(promise)));
  } else {
    TracingController::GetInstance()->GetTraceBufferUsage(
        base::BindOnce(&OnTraceBufferUsageAvailable, std::move(promise)));
  }
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
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_content_tracing, Initialize)
