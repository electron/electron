// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/crash_reporter/crash_reporter.h"

#include <memory>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/common/content_switches.h"
#include "electron/electron_version.h"
#include "shell/browser/browser.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"

namespace crash_reporter {

const char kCrashpadProcess[] = "crash-handler";
const char kCrashesDirectoryKey[] = "crashes-directory";

CrashReporter::CrashReporter() {
#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  bool run_as_node = base::Environment::Create()->HasVar(electron::kRunAsNode);
#else
  bool run_as_node = false;
#endif

  if (run_as_node) {
    process_type_ = "node";
  } else {
    auto* cmd = base::CommandLine::ForCurrentProcess();
    process_type_ = cmd->GetSwitchValueASCII(switches::kProcessType);
  }
  // process_type_ will be empty for browser process
}

CrashReporter::~CrashReporter() = default;

bool CrashReporter::IsInitialized() {
  return is_initialized_;
}

void CrashReporter::Start(const std::string& submit_url,
                          const base::FilePath& crashes_dir,
                          bool upload_to_server,
                          bool skip_system_crash_handler,
                          bool rate_limit,
                          bool compress,
                          const StringMap& extra_parameters) {
  is_initialized_ = true;
  SetUploadParameters(extra_parameters);

  Init(submit_url, crashes_dir, upload_to_server, skip_system_crash_handler,
       rate_limit, compress);
}

void CrashReporter::SetUploadParameters(const StringMap& parameters) {
  upload_parameters_ = parameters;
  upload_parameters_["process_type"] =
      process_type_.empty() ? "browser" : process_type_;
  upload_parameters_["prod"] = ELECTRON_PRODUCT_NAME;
  upload_parameters_["ver"] = ELECTRON_VERSION_STRING;

  // Setting platform dependent parameters.
  SetUploadParameters();
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
        result.emplace_back(report_time, report_item[1]);
      }
    }
  }
  return result;
}

std::map<std::string, std::string> CrashReporter::GetParameters() const {
  return upload_parameters_;
}

#if defined(OS_MACOSX) && defined(MAS_BUILD)
class DummyCrashReporter : public CrashReporter {
 public:
  ~DummyCrashReporter() override {}

  void SetUploadToServer(bool upload_to_server) override {}
  bool GetUploadToServer() override { return false; }
  void AddExtraParameter(const std::string& key,
                         const std::string& value) override {}
  void RemoveExtraParameter(const std::string& key) override {}

  void Init(const std::string& submit_url,
            const base::FilePath& crashes_dir,
            bool upload_to_server,
            bool skip_system_crash_handler,
            bool rate_limit,
            bool compress) override {}
  void SetUploadParameters() override {}
};

// static
CrashReporter* CrashReporter::GetInstance() {
  static DummyCrashReporter crash_reporter;
  return &crash_reporter;
}
#endif

void CrashReporter::StartInstance(const gin_helper::Dictionary& options) {
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
  bool rate_limit = false;
  options.Get("rateLimit", &rate_limit);
  bool compress = false;
  options.Get("compress", &compress);

  extra_parameters["_productName"] = product_name;
  extra_parameters["_companyName"] = company_name;

  bool upload_to_server = true;
  bool skip_system_crash_handler = false;

  reporter->Start(submit_url, crashes_dir, upload_to_server,
                  skip_system_crash_handler, rate_limit, compress,
                  extra_parameters);
}

}  // namespace crash_reporter
