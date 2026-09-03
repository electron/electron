// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_config.h"
#include "content/public/browser/tracing_controller.h"
#include "services/tracing/public/cpp/perfetto/perfetto_data_source_names.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "third_party/perfetto/protos/perfetto/config/chrome/sampling_heap_profiler.gen.h"
#include "third_party/perfetto/protos/perfetto/config/trace_config.gen.h"

using content::TracingController;
using namespace std::literals;

namespace {

struct HeapProfilerOptions {
  uint32_t sampling_interval_bytes = 128 * 1024;
  uint32_t sampling_interval_ms = 50;
};

struct ContentTracingConfig {
  base::trace_event::TraceConfig trace_config;
  std::optional<HeapProfilerOptions> heap_profiler_options;
};

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

    if (options.Has("sampling_interval_bytes") &&
        (!options.Get("sampling_interval_bytes",
                      &out->sampling_interval_bytes) ||
         out->sampling_interval_bytes == 0)) {
      return false;
    }
    return !options.Has("sampling_interval_ms") ||
           options.Get("sampling_interval_ms", &out->sampling_interval_ms);
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

void AddHeapProfilingDataSource(
    const base::trace_event::TraceConfig& trace_config,
    const HeapProfilerOptions& heap_profiler_options,
    perfetto::TraceConfig* perfetto_config) {
  auto* heap_data_source = perfetto_config->add_data_sources();
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
}

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
      std::move(promise), file_path.value_or(base::FilePath()));

  auto* instance = TracingController::GetInstance();
  if (!instance->IsTracing()) {
    std::move(resolve_or_reject)
        .Run("Failed to stop tracing - no trace in progress"sv);
  } else if (file_path) {
    auto split_callback = base::SplitOnceCallback(std::move(resolve_or_reject));
    // The file endpoint hands this closure to a thread pool sequence and, if
    // it fails to write the trace file, drops it there without running it. The
    // promise it owns must be destroyed on the thread that created it, so make
    // sure both running and destroying the closure happen back on this thread.
    auto endpoint = TracingController::CreateFileEndpoint(
        *file_path, base::BindPostTaskToCurrentDefault(base::BindOnce(
                        std::move(split_callback.first), ""sv)));
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
  if (instance->IsTracing()) {
    return gin_helper::Promise<void>::ResolvedPromise(isolate);
  }

  TracingController::StartTracingOptions options;
  if (config.heap_profiler_options) {
    options.output_format = TracingController::TraceDataFormat::kProtobuf;
    options.perfetto_config_modifier =
        base::BindOnce(&AddHeapProfilingDataSource, config.trace_config,
                       *config.heap_profiler_options);
  }

  if (!instance->StartTracing(
          config.trace_config,
          base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                         std::move(promise)),
          std::move(options))) {
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
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_content_tracing, Initialize)
