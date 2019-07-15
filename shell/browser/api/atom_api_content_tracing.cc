// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <set>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/optional.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/tracing_controller.h"
#include "native_mate/dictionary.h"
#include "shell/common/native_mate_converters/callback.h"
#include "shell/common/native_mate_converters/file_path_converter.h"
#include "shell/common/native_mate_converters/value_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/promise_util.h"

using content::TracingController;

namespace mate {

template <>
struct Converter<base::trace_event::TraceConfig> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::trace_event::TraceConfig* out) {
    // (alexeykuzmin): A combination of "categoryFilter" and "traceOptions"
    // has to be checked first because none of the fields
    // in the `memory_dump_config` dict below are mandatory
    // and we cannot check the config format.
    Dictionary options;
    if (ConvertFromV8(isolate, val, &options)) {
      std::string category_filter, trace_options;
      if (options.Get("categoryFilter", &category_filter) &&
          options.Get("traceOptions", &trace_options)) {
        *out = base::trace_event::TraceConfig(category_filter, trace_options);
        return true;
      }
    }

    base::DictionaryValue memory_dump_config;
    if (ConvertFromV8(isolate, val, &memory_dump_config)) {
      *out = base::trace_event::TraceConfig(memory_dump_config);
      return true;
    }

    return false;
  }
};

}  // namespace mate

namespace {

using CompletionCallback = base::OnceCallback<void(const base::FilePath&)>;

base::Optional<base::FilePath> CreateTemporaryFileOnIO() {
  base::FilePath temp_file_path;
  if (!base::CreateTemporaryFile(&temp_file_path))
    return base::nullopt;
  return base::make_optional(std::move(temp_file_path));
}

void StopTracing(electron::util::Promise promise,
                 base::Optional<base::FilePath> file_path) {
  if (file_path) {
    auto endpoint = TracingController::CreateFileEndpoint(
        *file_path,
        base::AdaptCallbackForRepeating(base::BindOnce(
            &electron::util::Promise::ResolvePromise<base::FilePath>,
            std::move(promise), *file_path)));
    TracingController::GetInstance()->StopTracing(endpoint);
  } else {
    promise.RejectWithErrorMessage(
        "Failed to create temporary file for trace data");
  }
}

v8::Local<v8::Promise> StopRecording(mate::Arguments* args) {
  electron::util::Promise promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  base::FilePath path;
  if (args->GetNext(&path) && !path.empty()) {
    StopTracing(std::move(promise), base::make_optional(path));
  } else {
    // use a temporary file.
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(CreateTemporaryFileOnIO),
        base::BindOnce(StopTracing, std::move(promise)));
  }

  return handle;
}

v8::Local<v8::Promise> GetCategories(v8::Isolate* isolate) {
  electron::util::Promise promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // Note: This method always succeeds.
  TracingController::GetInstance()->GetCategories(base::BindOnce(
      electron::util::Promise::ResolvePromise<const std::set<std::string>&>,
      std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> StartTracing(
    v8::Isolate* isolate,
    const base::trace_event::TraceConfig& trace_config) {
  electron::util::Promise promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!TracingController::GetInstance()->StartTracing(
          trace_config,
          base::BindOnce(electron::util::Promise::ResolveEmptyPromise,
                         std::move(promise)))) {
    // If StartTracing returns false, that means it didn't invoke its callback.
    // Return an already-resolved promise and abandon the previous promise (it
    // was std::move()d into the StartTracing callback and has been deleted by
    // this point).
    return electron::util::Promise::ResolvedPromise(isolate);
  }
  return handle;
}

void OnTraceBufferUsageAvailable(electron::util::Promise promise,
                                 float percent_full,
                                 size_t approximate_count) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(promise.isolate());
  dict.Set("percentage", percent_full);
  dict.Set("value", approximate_count);

  promise.Resolve(dict.GetHandle());
}

v8::Local<v8::Promise> GetTraceBufferUsage(v8::Isolate* isolate) {
  electron::util::Promise promise(isolate);
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
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCategories", &GetCategories);
  dict.SetMethod("startRecording", &StartTracing);
  dict.SetMethod("stopRecording", &StopRecording);
  dict.SetMethod("getTraceBufferUsage", &GetTraceBufferUsage);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_content_tracing, Initialize)
