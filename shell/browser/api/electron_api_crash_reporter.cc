// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_crash_reporter.h"

#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/chrome_paths.h"
#include "components/upload_list/crash_upload_list.h"
#include "components/upload_list/text_log_upload_list.h"
#include "content/public/common/content_switches.h"
#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "shell/common/electron_paths.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/time_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

#if !defined(MAS_BUILD)
#include "components/crash/core/app/crashpad.h"  // nogncheck
#include "components/crash/core/browser/crash_upload_list_crashpad.h"  // nogncheck
#include "components/crash/core/common/crash_key.h"
#include "shell/app/electron_crash_reporter_client.h"
#include "shell/common/crash_keys.h"
#include "third_party/crashpad/crashpad/client/crashpad_info.h"  // nogncheck
#endif

#if BUILDFLAG(IS_LINUX)
#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "components/crash/core/app/breakpad_linux.h"
#include "components/crash/core/common/crash_keys.h"
#include "components/upload_list/combining_upload_list.h"
#include "v8/include/v8-wasm-trap-handler-posix.h"
#include "v8/include/v8.h"
#endif

namespace {

#if BUILDFLAG(IS_LINUX)
std::map<std::string, std::string>& GetGlobalCrashKeysMutable() {
  static base::NoDestructor<std::map<std::string, std::string>>
      global_crash_keys;
  return *global_crash_keys;
}
#endif  // BUILDFLAG(IS_LINUX)

bool g_crash_reporter_initialized = false;

}  // namespace

namespace electron {

namespace api {

namespace crash_reporter {

#if defined(MAS_BUILD)
namespace {

void NoOp() {}

}  // namespace
#endif

bool IsCrashReporterEnabled() {
  return g_crash_reporter_initialized;
}

#if BUILDFLAG(IS_LINUX)
const std::map<std::string, std::string>& GetGlobalCrashKeys() {
  return GetGlobalCrashKeysMutable();
}

bool GetClientIdPath(base::FilePath* path) {
  if (base::PathService::Get(electron::DIR_CRASH_DUMPS, path)) {
    *path = path->Append("client_id");
    return true;
  }
  return false;
}

std::string ReadClientId() {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  std::string client_id;
  // "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".length == 36
  base::FilePath client_id_path;
  if (GetClientIdPath(&client_id_path) &&
      (!base::ReadFileToStringWithMaxSize(client_id_path, &client_id, 36) ||
       client_id.size() != 36))
    return std::string();
  return client_id;
}

void WriteClientId(const std::string& client_id) {
  DCHECK_EQ(client_id.size(), 36u);
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::FilePath client_id_path;
  if (GetClientIdPath(&client_id_path))
    base::WriteFile(client_id_path, client_id);
}

std::string GetClientId() {
  static base::NoDestructor<std::string> client_id;
  if (!client_id->empty())
    return *client_id;
  *client_id = ReadClientId();
  if (client_id->empty()) {
    *client_id = base::GenerateGUID();
    WriteClientId(*client_id);
  }
  return *client_id;
}
#endif

void Start(const std::string& submit_url,
           bool upload_to_server,
           bool ignore_system_crash_handler,
           bool rate_limit,
           bool compress,
           const std::map<std::string, std::string>& global_extra,
           const std::map<std::string, std::string>& extra,
           bool is_node_process) {
#if !defined(MAS_BUILD)
  if (g_crash_reporter_initialized)
    return;
  g_crash_reporter_initialized = true;
  ElectronCrashReporterClient::Create();
  ElectronCrashReporterClient::Get()->SetUploadUrl(submit_url);
  ElectronCrashReporterClient::Get()->SetCollectStatsConsent(upload_to_server);
  ElectronCrashReporterClient::Get()->SetShouldRateLimit(rate_limit);
  ElectronCrashReporterClient::Get()->SetShouldCompressUploads(compress);
  ElectronCrashReporterClient::Get()->SetGlobalAnnotations(global_extra);
  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type =
      is_node_process
          ? "node"
          : command_line->GetSwitchValueASCII(::switches::kProcessType);
#if BUILDFLAG(IS_LINUX)
  if (::crash_reporter::IsCrashpadEnabled()) {
    for (const auto& pair : extra)
      electron::crash_keys::SetCrashKey(pair.first, pair.second);
    {
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      ::crash_reporter::InitializeCrashpad(process_type.empty(), process_type);
    }
    if (ignore_system_crash_handler) {
      crashpad::CrashpadInfo::GetCrashpadInfo()
          ->set_system_crash_reporter_forwarding(crashpad::TriState::kDisabled);
    }
  } else {
    ::crash_keys::SetMetricsClientIdFromGUID(GetClientId());
    auto& global_crash_keys = GetGlobalCrashKeysMutable();
    for (const auto& pair : global_extra) {
      global_crash_keys[pair.first] = pair.second;
    }
    for (const auto& pair : extra)
      electron::crash_keys::SetCrashKey(pair.first, pair.second);
    for (const auto& pair : global_extra)
      electron::crash_keys::SetCrashKey(pair.first, pair.second);
    breakpad::InitCrashReporter(process_type);
  }
#elif BUILDFLAG(IS_MAC)
  for (const auto& pair : extra)
    electron::crash_keys::SetCrashKey(pair.first, pair.second);
  ::crash_reporter::InitializeCrashpad(process_type.empty(), process_type);
  if (ignore_system_crash_handler) {
    crashpad::CrashpadInfo::GetCrashpadInfo()
        ->set_system_crash_reporter_forwarding(crashpad::TriState::kDisabled);
  }
#elif BUILDFLAG(IS_WIN)
  for (const auto& pair : extra)
    electron::crash_keys::SetCrashKey(pair.first, pair.second);
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  ::crash_reporter::InitializeCrashpadWithEmbeddedHandler(
      process_type.empty(), process_type,
      base::WideToUTF8(user_data_dir.value()), base::FilePath());
#endif
#endif
}

}  // namespace crash_reporter

}  // namespace api

}  // namespace electron

namespace {

#if defined(MAS_BUILD)
void GetUploadedReports(
    v8::Isolate* isolate,
    base::OnceCallback<void(v8::Local<v8::Value>)> callback) {
  std::move(callback).Run(v8::Array::New(isolate));
}
#else
scoped_refptr<UploadList> CreateCrashUploadList() {
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  return base::MakeRefCounted<CrashUploadListCrashpad>();
#else
  base::FilePath crash_dir_path;
  base::PathService::Get(electron::DIR_CRASH_DUMPS, &crash_dir_path);
  base::FilePath upload_log_path =
      crash_dir_path.AppendASCII(CrashUploadList::kReporterLogFilename);
  scoped_refptr<UploadList> result =
      base::MakeRefCounted<TextLogUploadList>(upload_log_path);
  if (crash_reporter::IsCrashpadEnabled()) {
    // Crashpad keeps the records of C++ crashes (segfaults, etc) in its
    // internal database. The JavaScript error reporter writes JS error upload
    // records to the older text format. Combine the two to present a complete
    // list to the user.
    // TODO(nornagon): what is "The JavaScript error reporter", and do we care
    // about it?
    std::vector<scoped_refptr<UploadList>> uploaders = {
        base::MakeRefCounted<CrashUploadListCrashpad>(), std::move(result)};
    result = base::MakeRefCounted<CombiningUploadList>(std::move(uploaders));
  }
  return result;
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
}

v8::Local<v8::Value> GetUploadedReports(v8::Isolate* isolate) {
  auto list = CreateCrashUploadList();
  // TODO(nornagon): switch to using Load() instead of LoadSync() once the
  // synchronous version of getUploadedReports is deprecated so we can remove
  // our patch.
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    list->LoadSync();
  }
  std::vector<UploadList::UploadInfo> uploads;
  constexpr size_t kMaxUploadReportsToList = std::numeric_limits<size_t>::max();
  list->GetUploads(kMaxUploadReportsToList, &uploads);
  std::vector<v8::Local<v8::Object>> result;
  for (const auto& upload : uploads) {
    result.push_back(gin::DataObjectBuilder(isolate)
                         .Set("date", upload.upload_time)
                         .Set("id", upload.upload_id)
                         .Build());
  }
  v8::Local<v8::Value> v8_result = gin::ConvertToV8(isolate, result);
  return v8_result;
}
#endif

void SetUploadToServer(bool upload) {
#if !defined(MAS_BUILD)
  ElectronCrashReporterClient::Get()->SetCollectStatsConsent(upload);
#endif
}

bool GetUploadToServer() {
#if defined(MAS_BUILD)
  return false;
#else
  return ElectronCrashReporterClient::Get()->GetCollectStatsConsent();
#endif
}

v8::Local<v8::Value> GetParameters(v8::Isolate* isolate) {
  std::map<std::string, std::string> keys;
#if !defined(MAS_BUILD)
  electron::crash_keys::GetCrashKeys(&keys);
#endif
  return gin::ConvertToV8(isolate, keys);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("start", &electron::api::crash_reporter::Start);
#if defined(MAS_BUILD)
  dict.SetMethod("addExtraParameter", &electron::api::crash_reporter::NoOp);
  dict.SetMethod("removeExtraParameter", &electron::api::crash_reporter::NoOp);
#else
  dict.SetMethod("addExtraParameter", &electron::crash_keys::SetCrashKey);
  dict.SetMethod("removeExtraParameter", &electron::crash_keys::ClearCrashKey);
#endif
  dict.SetMethod("getParameters", &GetParameters);
  dict.SetMethod("getUploadedReports", &GetUploadedReports);
  dict.SetMethod("setUploadToServer", &SetUploadToServer);
  dict.SetMethod("getUploadToServer", &GetUploadToServer);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_crash_reporter, Initialize)
