// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <set>

#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/function_converter.h"
#include "base/bind.h"
#include "content/public/browser/tracing_controller.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

using content::TracingController;

namespace mate {

template<typename T>
struct Converter<std::set<T> > {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const std::set<T>& val) {
    v8::Handle<v8::Array> result(v8::Array::New(static_cast<int>(val.size())));
    typename std::set<T>::const_iterator it;
    int i;
    for (i = 0, it = val.begin(); it != val.end(); ++it, ++i)
      result->Set(i, Converter<T>::ToV8(isolate, *it));
    return result;
  }
};

template<>
struct Converter<TracingController::Options> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     TracingController::Options* out) {
    if (!val->IsNumber())
      return false;
    *out = static_cast<TracingController::Options>(val->IntegerValue());
    return true;
  }
};

namespace internal {

// Get object out of scoped_ptr, should be removed after upgraded to Chrome35.
template<typename P1>
struct V8FunctionInvoker<void(scoped_ptr<P1>)> {
  static void Go(v8::Isolate* isolate, SafeV8Function function,
      scoped_ptr<P1> a1) {
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, *a1),
    };
    holder->Call(holder, arraysize(args), args);
  }
};

}  // namespace internal

}  // namespace mate


namespace {

void EnableRecording(
    const std::string& category_filter,
    TracingController::Options options,
    const TracingController::EnableRecordingDoneCallback& callback) {
  TracingController::GetInstance()->EnableRecording(
      base::debug::CategoryFilter(category_filter), options, callback);
}

void EnableMonitoring(
    const std::string& category_filter,
    TracingController::Options options,
    const TracingController::EnableMonitoringDoneCallback& callback) {
  TracingController::GetInstance()->EnableMonitoring(
      base::debug::CategoryFilter(category_filter), options, callback);
}

void Initialize(v8::Handle<v8::Object> exports) {
  TracingController* controller = TracingController::GetInstance();
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.SetMethod("getCategories", base::Bind(
      &TracingController::GetCategories, base::Unretained(controller)));
  dict.SetMethod("_enableRecording", base::Bind(&EnableRecording));
  dict.SetMethod("disableRecording", base::Bind(
      &TracingController::DisableRecording, base::Unretained(controller)));
  dict.SetMethod("_enableMonitoring", base::Bind(&EnableMonitoring));
  dict.SetMethod("disableMonitoring", base::Bind(
      &TracingController::DisableMonitoring, base::Unretained(controller)));
  dict.SetMethod("captureCurrentMonitoringSnapshot", base::Bind(
      &TracingController::CaptureCurrentMonitoringSnapshot,
      base::Unretained(controller)));
}

}  // namespace

NODE_MODULE(atom_browser_tracing, Initialize)
