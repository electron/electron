// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <set>
#include <string>

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/promise_util.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "content/public/browser/tracing_controller.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

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

using CompletionCallback = base::Callback<void(const base::FilePath&)>;

scoped_refptr<TracingController::TraceDataEndpoint> GetTraceDataEndpoint(
    const base::FilePath& path,
    const CompletionCallback& callback) {
  base::FilePath result_file_path = path;
  if (result_file_path.empty() && !base::CreateTemporaryFile(&result_file_path))
    LOG(ERROR) << "Creating temporary file failed";

  return TracingController::CreateFileEndpoint(
      result_file_path, base::Bind(callback, result_file_path));
}

void OnRecordingStopped(scoped_refptr<atom::util::Promise> promise,
                        const base::FilePath& path) {
  promise->Resolve(path);
}

v8::Local<v8::Promise> StopRecording(v8::Isolate* isolate,
                                     const base::FilePath& path) {
  scoped_refptr<atom::util::Promise> promise = new atom::util::Promise(isolate);

  TracingController::GetInstance()->StopTracing(
      GetTraceDataEndpoint(path, base::Bind(&OnRecordingStopped, promise)));

  return promise->GetHandle();
}

void OnCategoriesAvailable(scoped_refptr<atom::util::Promise> promise,
                           const std::set<std::string>& categories) {
  promise->Resolve(categories);
}

v8::Local<v8::Promise> GetCategories(v8::Isolate* isolate) {
  scoped_refptr<atom::util::Promise> promise = new atom::util::Promise(isolate);
  bool success = TracingController::GetInstance()->GetCategories(
      base::BindOnce(&OnCategoriesAvailable, promise));

  if (!success)
    promise->RejectWithErrorMessage("Could not get categories.");

  return promise->GetHandle();
}

void OnTracingStarted(scoped_refptr<atom::util::Promise> promise) {
  promise->Resolve();
}

v8::Local<v8::Promise> StartTracing(
    v8::Isolate* isolate,
    const base::trace_event::TraceConfig& trace_config) {
  scoped_refptr<atom::util::Promise> promise = new atom::util::Promise(isolate);

  bool success = TracingController::GetInstance()->StartTracing(
      trace_config, base::BindOnce(&OnTracingStarted, promise));

  if (!success)
    promise->RejectWithErrorMessage("Could not start tracing");

  return promise->GetHandle();
}

void OnTraceBufferUsageAvailable(scoped_refptr<atom::util::Promise> promise,
                                 float percent_full,
                                 size_t approximate_count) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(promise->isolate());
  dict.Set("percentage", percent_full);
  dict.Set("value", approximate_count);

  promise->Resolve(dict.GetHandle());
}

v8::Local<v8::Promise> GetTraceBufferUsage(v8::Isolate* isolate) {
  scoped_refptr<atom::util::Promise> promise = new atom::util::Promise(isolate);
  bool success = TracingController::GetInstance()->GetTraceBufferUsage(
      base::BindOnce(&OnTraceBufferUsageAvailable, promise));

  if (!success)
    promise->RejectWithErrorMessage("Could not get trace buffer usage.");

  return promise->GetHandle();
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

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_content_tracing, Initialize)
