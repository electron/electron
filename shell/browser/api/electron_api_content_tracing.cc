// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <optional>
#include <set>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_config.h"
#include "content/public/browser/tracing_controller.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

using content::TracingController;

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

    base::Value::Dict memory_dump_config;
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
         const base::FilePath& path, std::optional<std::string> error) {
        if (error) {
          promise.RejectWithErrorMessage(error.value());
        } else {
          promise.Resolve(path);
        }
      },
      std::move(promise), *file_path);

  auto* instance = TracingController::GetInstance();
  if (!instance->IsTracing()) {
    std::move(resolve_or_reject)
        .Run(std::make_optional(
            "Failed to stop tracing - no trace in progress"));
  } else if (file_path) {
    auto split_callback = base::SplitOnceCallback(std::move(resolve_or_reject));
    auto endpoint = TracingController::CreateFileEndpoint(
        *file_path,
        base::BindOnce(std::move(split_callback.first), std::nullopt));
    if (!instance->StopTracing(endpoint)) {
      std::move(split_callback.second)
          .Run(std::make_optional("Failed to stop tracing"));
    }
  } else {
    std::move(resolve_or_reject)
        .Run(std::make_optional(
            "Failed to create temporary file for trace data"));
  }
}

v8::Local<v8::Promise> StopRecording(gin_helper::Arguments* args) {
  gin_helper::Promise<base::FilePath> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

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

  // Note: This method always succeeds.
  TracingController::GetInstance()->GetCategories(base::BindOnce(
      gin_helper::Promise<const std::set<std::string>&>::ResolvePromise,
      std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> StartTracing(
    v8::Isolate* isolate,
    const base::trace_event::TraceConfig& trace_config) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

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
  auto dict = gin_helper::Dictionary::CreateEmpty(promise.isolate());
  dict.Set("percentage", percent_full);
  dict.Set("value", approximate_count);

  promise.Resolve(dict);
}

v8::Local<v8::Promise> GetTraceBufferUsage(v8::Isolate* isolate) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // Note: This method always succeeds.
  TracingController::GetInstance()->GetTraceBufferUsage(
      base::BindOnce(&OnTraceBufferUsageAvailable, std::move(promise)));
  return handle;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCategories", &GetCategories);
  dict.SetMethod("startRecording", &StartTracing);
  dict.SetMethod("stopRecording", &StopRecording);
  dict.SetMethod("getTraceBufferUsage", &GetTraceBufferUsage);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_content_tracing, Initialize)
