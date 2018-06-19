// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/browser/net_log.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/values.h"
#include "content/public/common/content_switches.h"
#include "net/log/file_net_log_observer.h"
#include "net/log/net_log_util.h"

namespace brightray {

namespace {

std::unique_ptr<base::DictionaryValue> GetConstants() {
  std::unique_ptr<base::DictionaryValue> constants = net::GetNetConstants();

  // Adding client information to constants dictionary.
  auto client_info = std::make_unique<base::DictionaryValue>();
  client_info->SetString(
      "command_line",
      base::CommandLine::ForCurrentProcess()->GetCommandLineString());

  constants->Set("clientInfo", std::move(client_info));
  return constants;
}

}  // namespace

NetLog::NetLog() {}

NetLog::~NetLog() {
  StopDynamicLogging();
  StopLogging();
}

void NetLog::StartLogging() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kLogNetLog))
    return;

  base::FilePath log_path;
  log_path = command_line->GetSwitchValuePath(switches::kLogNetLog);
  if (log_path.empty())
    return;

  std::unique_ptr<base::Value> constants(GetConstants());  // Net constants
  net::NetLogCaptureMode capture_mode = net::NetLogCaptureMode::Default();

  file_net_log_observer_ =
      net::FileNetLogObserver::CreateUnbounded(log_path, std::move(constants));
  file_net_log_observer_->StartObserving(this, capture_mode);
}

void NetLog::StopLogging() {
  if (!file_net_log_observer_)
    return;

  file_net_log_observer_->StopObserving(nullptr, base::OnceClosure());
  file_net_log_observer_.reset();
}

void NetLog::StartDynamicLogging(const base::FilePath& log_path) {
  if (dynamic_file_net_log_observer_ || log_path.empty())
    return;

  dynamic_file_net_log_path_ = log_path;

  std::unique_ptr<base::Value> constants(GetConstants());  // Net constants
  net::NetLogCaptureMode capture_mode = net::NetLogCaptureMode::Default();

  dynamic_file_net_log_observer_ = net::FileNetLogObserver::CreateUnbounded(
      dynamic_file_net_log_path_, std::move(constants));
  dynamic_file_net_log_observer_->StartObserving(this, capture_mode);
}

bool NetLog::IsDynamicLogging() {
  return !!dynamic_file_net_log_observer_;
}

base::FilePath NetLog::GetDynamicLoggingPath() {
  return dynamic_file_net_log_path_;
}

void NetLog::StopDynamicLogging(base::OnceClosure callback) {
  if (!dynamic_file_net_log_observer_) {
    if (callback)
      std::move(callback).Run();
    return;
  }

  dynamic_file_net_log_observer_->StopObserving(nullptr, std::move(callback));
  dynamic_file_net_log_observer_.reset();

  dynamic_file_net_log_path_ = base::FilePath();
}

}  // namespace brightray
