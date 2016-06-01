// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main_delegate.h"

#include <string>
#include <iostream>

#include "atom/app/atom_content_client.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/common/google_api_key.h"
#include "atom/renderer/atom_renderer_client.h"
#include "atom/utility/atom_content_utility_client.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/environment.h"
#include "base/logging.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/common/content_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(OS_MACOSX)
#include "chrome/browser/mac/relauncher.h"
#endif

namespace atom {

namespace {

#if defined(OS_MACOSX)
const char* kRelauncherProcess = "relauncher";
#endif

bool IsBrowserProcess(base::CommandLine* cmd) {
  std::string process_type = cmd->GetSwitchValueASCII(switches::kProcessType);
  return process_type.empty();
}

#if defined(OS_WIN)
void InvalidParameterHandler(const wchar_t*, const wchar_t*, const wchar_t*,
                             unsigned int, uintptr_t) {
  // noop.
}
#endif

}  // namespace

AtomMainDelegate::AtomMainDelegate() {
}

AtomMainDelegate::~AtomMainDelegate() {
}

bool AtomMainDelegate::BasicStartupComplete(int* exit_code) {
  auto command_line = base::CommandLine::ForCurrentProcess();

  logging::LoggingSettings settings;
#if defined(OS_WIN)
  // On Windows the terminal returns immediately, so we add a new line to
  // prevent output in the same line as the prompt.
  if (IsBrowserProcess(command_line))
    std::wcout << std::endl;
#if defined(DEBUG)
  // Print logging to debug.log on Windows
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file = L"debug.log";
  settings.lock_log = logging::LOCK_LOG_FILE;
  settings.delete_old = logging::DELETE_OLD_LOG_FILE;
#else
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
#endif  // defined(DEBUG)
#else  // defined(OS_WIN)
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
#endif  // !defined(OS_WIN)

  // Only enable logging when --enable-logging is specified.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (!command_line->HasSwitch(switches::kEnableLogging) &&
      !env->HasVar("ELECTRON_ENABLE_LOGGING")) {
    settings.logging_dest = logging::LOG_NONE;
    logging::SetMinLogLevel(logging::LOG_NUM_SEVERITIES);
  }

  logging::InitLogging(settings);

  // Logging with pid and timestamp.
  logging::SetLogItems(true, false, true, false);

  // Enable convient stack printing.
  bool enable_stack_dumping = env->HasVar("ELECTRON_ENABLE_STACK_DUMPING");
#if defined(DEBUG) && defined(OS_LINUX)
  enable_stack_dumping = true;
#endif
  if (enable_stack_dumping)
    base::debug::EnableInProcessStackDumping();

  chrome::RegisterPathProvider();

#if defined(OS_MACOSX)
  SetUpBundleOverrides();
#endif

#if defined(OS_WIN)
  // Ignore invalid parameter errors.
  _set_invalid_parameter_handler(InvalidParameterHandler);
#endif

  return brightray::MainDelegate::BasicStartupComplete(exit_code);
}

void AtomMainDelegate::PreSandboxStartup() {
  brightray::MainDelegate::PreSandboxStartup();

  // Set google API key.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (!env->HasVar("GOOGLE_API_KEY"))
    env->SetVar("GOOGLE_API_KEY", GOOGLEAPIS_API_KEY);

  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line->GetSwitchValueASCII(
      switches::kProcessType);

  // Only append arguments for browser process.
  if (!IsBrowserProcess(command_line))
    return;

  // Disable renderer sandbox for most of node's functions.
  command_line->AppendSwitch(switches::kNoSandbox);

  // Allow file:// URIs to read other file:// URIs by default.
  command_line->AppendSwitch(switches::kAllowFileAccessFromFiles);

#if defined(OS_MACOSX)
  // Enable AVFoundation.
  command_line->AppendSwitch("enable-avfoundation");
#endif
}

content::ContentBrowserClient* AtomMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new AtomBrowserClient);
  return browser_client_.get();
}

content::ContentRendererClient*
    AtomMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new AtomRendererClient);
  return renderer_client_.get();
}

content::ContentUtilityClient* AtomMainDelegate::CreateContentUtilityClient() {
  utility_client_.reset(new AtomContentUtilityClient);
  return utility_client_.get();
}

#if defined(OS_MACOSX)
int AtomMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  if (process_type == kRelauncherProcess)
    return mac_relauncher::internal::RelauncherMain(main_function_params);
  else
    return -1;
}

bool AtomMainDelegate::ShouldSendMachPort(const std::string& process_type) {
  return process_type != kRelauncherProcess;
}

bool AtomMainDelegate::DelaySandboxInitialization(
    const std::string& process_type) {
  return process_type == kRelauncherProcess;
}
#endif

std::unique_ptr<brightray::ContentClient>
AtomMainDelegate::CreateContentClient() {
  return std::unique_ptr<brightray::ContentClient>(new AtomContentClient);
}

}  // namespace atom
