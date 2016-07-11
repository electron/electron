// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/net_log.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/values.h"
#include "content/public/common/content_switches.h"
#include "net/log/net_log_util.h"

namespace brightray {

namespace {

std::unique_ptr<base::DictionaryValue> GetConstants() {
  std::unique_ptr<base::DictionaryValue> constants = net::GetNetConstants();

  // Adding client information to constants dictionary.
  auto* client_info = new base::DictionaryValue();
  client_info->SetString(
      "command_line",
      base::CommandLine::ForCurrentProcess()->GetCommandLineString());

  constants->Set("clientInfo", client_info);
  return constants;
}

}  // namespace

NetLog::NetLog() {
}

NetLog::~NetLog() {
}

void NetLog::StartLogging(net::URLRequestContext* url_request_context) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kLogNetLog))
    return;

  base::FilePath log_path = command_line->GetSwitchValuePath(switches::kLogNetLog);
#if defined(OS_WIN)
  log_file_.reset(_wfopen(log_path.value().c_str(), L"w"));
#elif defined(OS_POSIX)
  log_file_.reset(fopen(log_path.value().c_str(), "w"));
#endif

  if (!log_file_) {
    LOG(ERROR) << "Could not open file: " << log_path.value()
               << "for net logging";
    return;
  }

  std::unique_ptr<base::Value> constants(GetConstants());
  write_to_file_observer_.StartObserving(this,
                                         std::move(log_file_),
                                         constants.get(),
                                         url_request_context);
}

}  // namespace brightray
