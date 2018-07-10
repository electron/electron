// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/crash_reporter.h"

#include "atom/browser/browser.h"
#include "atom/common/atom_version.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/common/content_switches.h"

namespace crash_reporter {

CrashReporter::CrashReporter() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  is_browser_ = cmd->GetSwitchValueASCII(switches::kProcessType).empty();
}

CrashReporter::~CrashReporter() {}

void CrashReporter::Start(const std::string& product_name,
                          const std::string& company_name,
                          const std::string& submit_url,
                          const base::FilePath& crashes_dir,
                          bool upload_to_server,
                          bool skip_system_crash_handler,
                          const StringMap& extra_parameters) {
  SetUploadParameters(extra_parameters);

  InitBreakpad(product_name, ATOM_VERSION_STRING, company_name, submit_url,
               crashes_dir, upload_to_server, skip_system_crash_handler);
}

void CrashReporter::SetUploadParameters(const StringMap& parameters) {
  upload_parameters_ = parameters;
  upload_parameters_["process_type"] = is_browser_ ? "browser" : "renderer";

  // Setting platform dependent parameters.
  SetUploadParameters();
}

void CrashReporter::SetUploadToServer(const bool upload_to_server) {}

bool CrashReporter::GetUploadToServer() {
  return true;
}

std::vector<CrashReporter::UploadReportResult>
CrashReporter::GetUploadedReports(const base::FilePath& crashes_dir) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  std::string file_content;
  std::vector<CrashReporter::UploadReportResult> result;
  base::FilePath uploads_path =
      crashes_dir.Append(FILE_PATH_LITERAL("uploads.log"));
  if (base::ReadFileToString(uploads_path, &file_content)) {
    std::vector<std::string> reports = base::SplitString(
        file_content, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    for (const std::string& report : reports) {
      std::vector<std::string> report_item = base::SplitString(
          report, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
      int report_time = 0;
      if (report_item.size() >= 2 &&
          base::StringToInt(report_item[0], &report_time)) {
        result.push_back(
            CrashReporter::UploadReportResult(report_time, report_item[1]));
      }
    }
  }
  return result;
}

void CrashReporter::InitBreakpad(const std::string& product_name,
                                 const std::string& version,
                                 const std::string& company_name,
                                 const std::string& submit_url,
                                 const base::FilePath& crashes_dir,
                                 bool auto_submit,
                                 bool skip_system_crash_handler) {}

void CrashReporter::SetUploadParameters() {}

void CrashReporter::AddExtraParameter(const std::string& key,
                                      const std::string& value) {}

void CrashReporter::RemoveExtraParameter(const std::string& key) {}

std::map<std::string, std::string> CrashReporter::GetParameters() const {
  return upload_parameters_;
}

#if defined(OS_MACOSX) && defined(MAS_BUILD)
// static
CrashReporter* CrashReporter::GetInstance() {
  static CrashReporter crash_reporter;
  return &crash_reporter;
}
#endif

void CrashReporter::StartInstance(const mate::Dictionary& options) {
  auto* reporter = GetInstance();
  if (!reporter)
    return;

  std::string product_name;
  options.Get("productName", &product_name);
  std::string company_name;
  options.Get("companyName", &company_name);
  std::string submit_url;
  options.Get("submitURL", &submit_url);
  base::FilePath crashes_dir;
  options.Get("crashesDirectory", &crashes_dir);
  StringMap extra_parameters;
  options.Get("extra", &extra_parameters);

  extra_parameters["_productName"] = product_name;
  extra_parameters["_companyName"] = company_name;

  reporter->Start(product_name, company_name, submit_url, crashes_dir, true,
                  false, extra_parameters);
}

}  // namespace crash_reporter
