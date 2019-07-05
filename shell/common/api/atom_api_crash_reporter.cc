// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "base/bind.h"
#include "gin/data_object_builder.h"
#include "gin/dictionary.h"
#include "shell/common/crash_reporter/crash_reporter.h"
#include "shell/common/gin_util.h"
#include "shell/common/native_mate_converters/file_path_converter.h"
#include "shell/common/native_mate_converters/map_converter.h"

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
  using gin_util::SetMethod;
  auto reporter = base::Unretained(CrashReporter::GetInstance());
  SetMethod(exports, "start",
            base::BindRepeating(&CrashReporter::Start, reporter));
  SetMethod(exports, "addExtraParameter",
            base::BindRepeating(&CrashReporter::AddExtraParameter, reporter));
  SetMethod(
      exports, "removeExtraParameter",
      base::BindRepeating(&CrashReporter::RemoveExtraParameter, reporter));
  SetMethod(exports, "getParameters",
            base::BindRepeating(&CrashReporter::GetParameters, reporter));
  SetMethod(exports, "getUploadedReports",
            base::BindRepeating(&CrashReporter::GetUploadedReports, reporter));
  SetMethod(exports, "setUploadToServer",
            base::BindRepeating(&CrashReporter::SetUploadToServer, reporter));
  SetMethod(exports, "getUploadToServer",
            base::BindRepeating(&CrashReporter::GetUploadToServer, reporter));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_common_crash_reporter, Initialize)
