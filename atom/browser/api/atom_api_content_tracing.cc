// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <set>
#include <string>

#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/bind.h"
#include "content/public/browser/tracing_controller.h"
#include "native_mate/callback.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

using content::TracingController;

namespace mate {

template<>
struct Converter<base::trace_event::CategoryFilter> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::trace_event::CategoryFilter* out) {
    std::string filter;
    if (!ConvertFromV8(isolate, val, &filter))
      return false;
    *out = base::trace_event::CategoryFilter(filter);
    return true;
  }
};

template<>
struct Converter<base::trace_event::TraceOptions> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::trace_event::TraceOptions* out) {
    std::string options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;
    return out->SetFromString(options);
  }
};

}  // namespace mate

namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  auto controller = base::Unretained(TracingController::GetInstance());
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCategories", base::Bind(
      &TracingController::GetCategories, controller));
  dict.SetMethod("startRecording", base::Bind(
      &TracingController::EnableRecording, controller));
  dict.SetMethod("stopRecording", base::Bind(
      &TracingController::DisableRecording, controller, nullptr));
  dict.SetMethod("startMonitoring", base::Bind(
      &TracingController::EnableMonitoring, controller));
  dict.SetMethod("stopMonitoring", base::Bind(
      &TracingController::DisableMonitoring, controller));
  dict.SetMethod("captureMonitoringSnapshot", base::Bind(
      &TracingController::CaptureMonitoringSnapshot, controller, nullptr));
  dict.SetMethod("getTraceBufferUsage", base::Bind(
      &TracingController::GetTraceBufferUsage, controller));
  dict.SetMethod("setWatchEvent", base::Bind(
      &TracingController::SetWatchEvent, controller));
  dict.SetMethod("cancelWatchEvent", base::Bind(
      &TracingController::CancelWatchEvent, controller));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_content_tracing, Initialize)
