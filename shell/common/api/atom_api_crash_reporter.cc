// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "base/bind.h"
#include "gin/data_object_builder.h"
#include "shell/common/crash_reporter/crash_reporter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"

#include "shell/common/node_includes.h"

using crash_reporter::CrashReporter;

namespace gin {

template <>
struct Converter<CrashReporter::UploadReportResult> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const CrashReporter::UploadReportResult& reports) {
    return gin::DataObjectBuilder(isolate)
        .Set("date",
             v8::Date::New(isolate->GetCurrentContext(), reports.first * 1000.0)
                 .ToLocalChecked())
        .Set("id", reports.second)
        .Build();
  }
};

}  // namespace gin

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  auto reporter = base::Unretained(CrashReporter::GetInstance());
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("start", base::BindRepeating(&CrashReporter::Start, reporter));
  dict.SetMethod(
      "addExtraParameter",
      base::BindRepeating(&CrashReporter::AddExtraParameter, reporter));
  dict.SetMethod(
      "removeExtraParameter",
      base::BindRepeating(&CrashReporter::RemoveExtraParameter, reporter));
  dict.SetMethod("getParameters",
                 base::BindRepeating(&CrashReporter::GetParameters, reporter));
  dict.SetMethod(
      "getUploadedReports",
      base::BindRepeating(&CrashReporter::GetUploadedReports, reporter));
  dict.SetMethod(
      "setUploadToServer",
      base::BindRepeating(&CrashReporter::SetUploadToServer, reporter));
  dict.SetMethod(
      "getUploadToServer",
      base::BindRepeating(&CrashReporter::GetUploadToServer, reporter));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_common_crash_reporter, Initialize)
