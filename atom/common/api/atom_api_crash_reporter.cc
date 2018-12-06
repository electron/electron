// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "atom/common/crash_reporter/crash_reporter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/bind.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

using crash_reporter::CrashReporter;

namespace mate {

template <>
struct Converter<CrashReporter::UploadReportResult> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const CrashReporter::UploadReportResult& reports) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("date",
             v8::Date::New(isolate->GetCurrentContext(), reports.first * 1000.0)
                 .ToLocalChecked());
    dict.Set("id", reports.second);
    return dict.GetHandle();
  }
};

}  // namespace mate

namespace {

void AddExtraParameter(const std::string& key, const std::string& value) {
  CrashReporter::GetInstance()->AddExtraParameter(key, value);
}

void RemoveExtraParameter(const std::string& key) {
  CrashReporter::GetInstance()->RemoveExtraParameter(key);
}

std::map<std::string, std::string> GetParameters() {
  return CrashReporter::GetInstance()->GetParameters();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  auto reporter = base::Unretained(CrashReporter::GetInstance());
  dict.SetMethod("start", base::Bind(&CrashReporter::Start, reporter));
  dict.SetMethod("addExtraParameter", &AddExtraParameter);
  dict.SetMethod("removeExtraParameter", &RemoveExtraParameter);
  dict.SetMethod("getParameters", &GetParameters);
  dict.SetMethod("getUploadedReports",
                 base::Bind(&CrashReporter::GetUploadedReports, reporter));
  dict.SetMethod("setUploadToServer",
                 base::Bind(&CrashReporter::SetUploadToServer, reporter));
  dict.SetMethod("getUploadToServer",
                 base::Bind(&CrashReporter::GetUploadToServer, reporter));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_common_crash_reporter, Initialize)
