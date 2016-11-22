// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <set>
#include <string>

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "content/public/browser/tracing_controller.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

using content::TracingController;

namespace mate {

template<>
struct Converter<base::trace_event::TraceConfig> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::trace_event::TraceConfig* out) {
    Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;
    std::string category_filter, trace_options;
    if (!options.Get("categoryFilter", &category_filter) ||
        !options.Get("traceOptions", &trace_options))
      return false;
    *out = base::trace_event::TraceConfig(category_filter, trace_options);
    return true;
  }
};

}  // namespace mate

namespace {

using CompletionCallback = base::Callback<void(const base::FilePath&)>;

scoped_refptr<TracingController::TraceDataSink> GetTraceDataSink(
    const base::FilePath& path, const CompletionCallback& callback) {
  base::FilePath result_file_path = path;
  if (result_file_path.empty() && !base::CreateTemporaryFile(&result_file_path))
    LOG(ERROR) << "Creating temporary file failed";

  return TracingController::CreateFileSink(result_file_path,
                                           base::Bind(callback,
                                                      result_file_path));
}

void StopRecording(const base::FilePath& path,
                   const CompletionCallback& callback) {
  TracingController::GetInstance()->StopTracing(
      GetTraceDataSink(path, callback));
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  auto controller = base::Unretained(TracingController::GetInstance());
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCategories", base::Bind(
      &TracingController::GetCategories, controller));
  dict.SetMethod("startRecording", base::Bind(
      &TracingController::StartTracing, controller));
  dict.SetMethod("stopRecording", &StopRecording);
  dict.SetMethod("getTraceBufferUsage", base::Bind(
      &TracingController::GetTraceBufferUsage, controller));
  dict.SetMethod("setWatchEvent", base::Bind(
      &TracingController::SetWatchEvent, controller));
  dict.SetMethod("cancelWatchEvent", base::Bind(
      &TracingController::CancelWatchEvent, controller));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_content_tracing, Initialize)
