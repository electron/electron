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
  StopLogging();
}

/**
 * Starts logging to the filepath remembered, otherwise to the default specified
 * via --log-net-log.
 */
void NetLog::StartLogging() {
  if (file_net_log_observer_)
    return;

  if (file_net_log_path_.empty()) {
    auto* command_line = base::CommandLine::ForCurrentProcess();
    if (!command_line->HasSwitch(switches::kLogNetLog)) {
      return;
    }
    file_net_log_path_ = command_line->GetSwitchValuePath(switches::kLogNetLog);
  }

  std::unique_ptr<base::Value> constants(GetConstants());  // Net constants
  net::NetLogCaptureMode capture_mode = net::NetLogCaptureMode::Default();

  file_net_log_observer_ = net::FileNetLogObserver::CreateUnbounded(
      file_net_log_path_, std::move(constants));
  file_net_log_observer_->StartObserving(this, capture_mode);
}

void NetLog::StartLogging(const base::FilePath& log_path) {
  // Cannot set path when already logging
  if (file_net_log_observer_ || log_path.empty())
    return;

  file_net_log_path_ = log_path;
  StartLogging();
}

bool NetLog::IsLogging() {
  return !!file_net_log_observer_;
}

void NetLog::StopLogging(base::OnceClosure callback) {
  if (!file_net_log_observer_) {
    // Immediate callback
    std::move(callback).Run();
    return;
  }

  file_net_log_observer_->StopObserving(nullptr, std::move(callback));
  file_net_log_observer_.reset();
}

}  // namespace brightray
