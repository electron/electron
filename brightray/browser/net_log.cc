// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/browser/net_log.h"

#include <utility>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "content/public/common/content_switches.h"
#include "net/log/file_net_log_observer.h"
#include "net/log/net_log_util.h"

namespace brightray {

namespace {

std::unique_ptr<base::DictionaryValue> GetConstants() {
  std::unique_ptr<base::DictionaryValue> constants = net::GetNetConstants();

  // Adding client information to constants dictionary.
  auto client_info = base::MakeUnique<base::DictionaryValue>();
  client_info->SetString(
      "command_line",
      base::CommandLine::ForCurrentProcess()->GetCommandLineString());

  constants->Set("clientInfo", std::move(client_info));
  return constants;
}

}  // namespace

NetLog::NetLog() {
}

NetLog::~NetLog() {
  if (file_net_log_observer_) {
    file_net_log_observer_->StopObserving(nullptr, base::Closure());
    file_net_log_observer_.reset();
  }
}

void NetLog::StartLogging() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kLogNetLog))
    return;

  base::FilePath log_path =
      command_line->GetSwitchValuePath(switches::kLogNetLog);
  std::unique_ptr<base::Value> constants(GetConstants());
  net::NetLogCaptureMode capture_mode =
      net::NetLogCaptureMode::IncludeCookiesAndCredentials();

  file_net_log_observer_ =
      net::FileNetLogObserver::CreateUnbounded(log_path, std::move(constants));
  file_net_log_observer_->StartObserving(this, capture_mode);
}

}  // namespace brightray
