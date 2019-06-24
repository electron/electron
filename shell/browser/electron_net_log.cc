// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "shell/browser/electron_net_log.h"
#include <memory>
#include <utility>
#include "base/callback.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/system/sys_info.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/version_info/version_info.h"
#include "net/log/file_net_log_observer.h"
#include "net/log/net_log_util.h"
namespace net_log {
ElectronNetLog::ElectronNetLog() {}
ElectronNetLog::~ElectronNetLog() {
  ClearFileNetLogObserver();
}
void ElectronNetLog::StartWritingToFile(
    const base::FilePath& path,
    net::NetLogCaptureMode capture_mode,
    const base::CommandLine::StringType& command_line_string,
    const std::string& channel_string) {
  DCHECK(!path.empty());
  // TODO(739485): The log file does not contain about:flags data.
  file_net_log_observer_ = net::FileNetLogObserver::CreateUnbounded(
      path, GetConstants(command_line_string, channel_string));
  file_net_log_observer_->StartObserving(this, capture_mode);
}
// static
std::unique_ptr<base::Value> ElectronNetLog::GetConstants(
    const base::CommandLine::StringType& command_line_string,
    const std::string& channel_string) {
  std::unique_ptr<base::DictionaryValue> constants_dict =
      net::GetNetConstants();
  DCHECK(constants_dict);
  auto platform_dict =
      GetPlatformConstants(command_line_string, channel_string);
  if (platform_dict)
    constants_dict->MergeDictionary(platform_dict.get());
  return constants_dict;
}
std::unique_ptr<base::DictionaryValue> ElectronNetLog::GetPlatformConstants(
    const base::CommandLine::StringType& command_line_string,
    const std::string& channel_string) {
  auto constants_dict = std::make_unique<base::DictionaryValue>();
  // Add a dictionary with the version of the client and its command line
  // arguments.
  auto dict = std::make_unique<base::DictionaryValue>();
  // We have everything we need to send the right values.
  dict->SetString("name", version_info::GetProductName());
  dict->SetString("version", version_info::GetVersionNumber());
  dict->SetString("cl", version_info::GetLastChange());
  dict->SetString("version_mod", channel_string);
  dict->SetString("official",
                  version_info::IsOfficialBuild() ? "official" : "unofficial");
  std::string os_type = base::StringPrintf(
      "%s: %s (%s)", base::SysInfo::OperatingSystemName().c_str(),
      base::SysInfo::OperatingSystemVersion().c_str(),
      base::SysInfo::OperatingSystemArchitecture().c_str());
  dict->SetString("os_type", os_type);
  dict->SetString("command_line", command_line_string);
  constants_dict->Set("clientInfo", std::move(dict));
  return constants_dict;
}
void ElectronNetLog::ShutDownBeforeThreadPool() {
  // TODO(eroman): Stop in-progress net_export_file_writer_ or delete its files?
  ClearFileNetLogObserver();
}
void ElectronNetLog::ClearFileNetLogObserver() {
  if (!file_net_log_observer_)
    return;
  // TODO(739487): The log file does not contain any polled data.
  //
  // TODO(eroman): FileNetLogObserver::StopObserving() posts to the file task
  // runner to finish writing the log file. Despite that sequenced task runner
  // being marked BLOCK_SHUTDOWN, those tasks are not actually running.
  //
  // This isn't a big deal when using the unbounded logger since the log
  // loading code can handle such truncated logs. But this will need fixing
  // if switching to log formats that are not so versatile (also if adding
  // polled data).
  file_net_log_observer_->StopObserving(nullptr /*polled_data*/,
                                        base::Closure());
  file_net_log_observer_.reset();
}
}  // namespace net_log