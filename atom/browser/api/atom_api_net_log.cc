// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_net_log.h"

#include <utility>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/system_network_context_manager.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/command_line.h"
#include "chrome/browser/browser_process.h"
#include "components/net_log/chrome_net_log.h"
#include "content/public/browser/storage_partition.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "net/url_request/url_request_context_getter.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

NetLog::NetLog(v8::Isolate* isolate, AtomBrowserContext* browser_context)
    : browser_context_(browser_context) {
  Init(isolate);

  net_log_writer_ = g_browser_process->system_network_context_manager()
                        ->GetNetExportFileWriter();
  net_log_writer_->AddObserver(this);
}

NetLog::~NetLog() {
  net_log_writer_->RemoveObserver(this);
}

void NetLog::StartLogging(mate::Arguments* args) {
  base::FilePath log_path;
  if (!args->GetNext(&log_path) || log_path.empty()) {
    args->ThrowError("The first parameter must be a valid string");
    return;
  }

  auto* network_context =
      content::BrowserContext::GetDefaultStoragePartition(browser_context_)
          ->GetNetworkContext();

  // TODO(deepak1556): Provide more flexibility to this module
  // by allowing customizations on the capturing options.
  net_log_writer_->StartNetLog(
      log_path, net::NetLogCaptureMode::Default(),
      net_log::NetExportFileWriter::kNoLimit /* file size limit */,
      base::CommandLine::ForCurrentProcess()->GetCommandLineString(),
      std::string(), network_context);
}

std::string NetLog::GetLoggingState() const {
  if (!net_log_state_)
    return std::string();
  const base::Value* current_log_state =
      net_log_state_->FindKeyOfType("state", base::Value::Type::STRING);
  if (!current_log_state)
    return std::string();
  return current_log_state->GetString();
}

bool NetLog::IsCurrentlyLogging() const {
  const std::string log_state = GetLoggingState();
  return (log_state == "STARTING_LOG") || (log_state == "LOGGING");
}

std::string NetLog::GetCurrentlyLoggingPath() const {
  // Net log exporter has a default path which will be used
  // when no log path is provided, but since we don't allow
  // net log capture without user provided file path, this
  // check is completely safe.
  if (IsCurrentlyLogging()) {
    const base::Value* current_log_path =
        net_log_state_->FindKeyOfType("file", base::Value::Type::STRING);
    if (current_log_path)
      return current_log_path->GetString();
  }

  return std::string();
}

void NetLog::StopLogging(mate::Arguments* args) {
  net_log::NetExportFileWriter::FilePathCallback callback;
  if (!args->GetNext(&callback)) {
    args->ThrowError("Invalid callback function");
    return;
  }

  if (IsCurrentlyLogging()) {
    stop_callback_queue_.emplace_back(callback);
    net_log_writer_->StopNetLog(nullptr);
  } else {
    callback.Run(base::FilePath());
  }
}

void NetLog::OnNewState(const base::DictionaryValue& state) {
  net_log_state_ = state.CreateDeepCopy();

  if (stop_callback_queue_.empty())
    return;

  if (GetLoggingState() == "NOT_LOGGING") {
    for (auto& callback : stop_callback_queue_) {
      if (!callback.is_null())
        net_log_writer_->GetFilePathToCompletedLog(callback);
    }
    stop_callback_queue_.clear();
  }
}

// static
mate::Handle<NetLog> NetLog::Create(v8::Isolate* isolate,
                                    AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new NetLog(isolate, browser_context));
}

// static
void NetLog::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "NetLog"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("currentlyLogging", &NetLog::IsCurrentlyLogging)
      .SetProperty("currentlyLoggingPath", &NetLog::GetCurrentlyLoggingPath)
      .SetMethod("startLogging", &NetLog::StartLogging)
      .SetMethod("stopLogging", &NetLog::StopLogging);
}

}  // namespace api

}  // namespace atom
