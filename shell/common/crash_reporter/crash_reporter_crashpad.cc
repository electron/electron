// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/crash_reporter/crash_reporter_crashpad.h"

#include <algorithm>
#include <memory>

#include "base/files/file_util.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "third_party/crashpad/crashpad/client/settings.h"

namespace crash_reporter {

CrashReporterCrashpad::CrashReporterCrashpad() {}

CrashReporterCrashpad::~CrashReporterCrashpad() {}

bool CrashReporterCrashpad::GetUploadToServer() {
  bool enabled = true;
  if (database_) {
    database_->GetSettings()->GetUploadsEnabled(&enabled);
  }
  return enabled;
}

void CrashReporterCrashpad::SetUploadToServer(const bool upload_to_server) {
  if (database_) {
    database_->GetSettings()->SetUploadsEnabled(upload_to_server);
  }
}

void CrashReporterCrashpad::SetCrashKeyValue(base::StringPiece key,
                                             base::StringPiece value) {
  simple_string_dictionary_->SetKeyValue(key.data(), value.data());
}

void CrashReporterCrashpad::SetInitialCrashKeyValues() {
  for (const auto& upload_parameter : upload_parameters_)
    SetCrashKeyValue(upload_parameter.first, upload_parameter.second);
}

void CrashReporterCrashpad::AddExtraParameter(const std::string& key,
                                              const std::string& value) {
  if (simple_string_dictionary_) {
    SetCrashKeyValue(key, value);
  } else {
    upload_parameters_[key] = value;
  }
}

void CrashReporterCrashpad::RemoveExtraParameter(const std::string& key) {
  if (simple_string_dictionary_)
    simple_string_dictionary_->RemoveKey(key.data());
  else
    upload_parameters_.erase(key);
}

std::map<std::string, std::string> CrashReporterCrashpad::GetParameters()
    const {
  if (simple_string_dictionary_) {
    std::map<std::string, std::string> ret;
    crashpad::SimpleStringDictionary::Iterator iter(*simple_string_dictionary_);
    for (;;) {
      auto* const entry = iter.Next();
      if (!entry)
        break;
      ret[entry->key] = entry->value;
    }
    return ret;
  }
  return upload_parameters_;
}

std::vector<CrashReporter::UploadReportResult>
CrashReporterCrashpad::GetUploadedReports(const base::FilePath& crashes_dir) {
  std::vector<CrashReporter::UploadReportResult> uploaded_reports;

  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    if (!base::PathExists(crashes_dir)) {
      return uploaded_reports;
    }
  }
  // Load crashpad database.
  std::unique_ptr<crashpad::CrashReportDatabase> database =
      crashpad::CrashReportDatabase::Initialize(crashes_dir);
  DCHECK(database);

  std::vector<crashpad::CrashReportDatabase::Report> completed_reports;
  crashpad::CrashReportDatabase::OperationStatus status =
      database->GetCompletedReports(&completed_reports);
  if (status != crashpad::CrashReportDatabase::kNoError) {
    return uploaded_reports;
  }

  for (const crashpad::CrashReportDatabase::Report& completed_report :
       completed_reports) {
    if (completed_report.uploaded) {
      uploaded_reports.push_back(
          UploadReportResult(static_cast<int>(completed_report.creation_time),
                             completed_report.id));
    }
  }

  auto sort_by_time = [](const UploadReportResult& a,
                         const UploadReportResult& b) {
    return a.first > b.first;
  };
  std::sort(uploaded_reports.begin(), uploaded_reports.end(), sort_by_time);
  return uploaded_reports;
}

}  // namespace crash_reporter
