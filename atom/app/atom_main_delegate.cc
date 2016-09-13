// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main_delegate.h"

#include <string>
#include <iostream>
#include <vector>

#include "atom/app/atom_content_client.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/relauncher.h"
#include "atom/common/google_api_key.h"
#include "atom/renderer/atom_renderer_client.h"
#include "atom/utility/atom_content_utility_client.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/environment.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/common/content_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_switches.h"

#if defined(ENABLE_EXTENSIONS)
#include "brave/browser/brave_content_browser_client.h"
#include "brave/renderer/brave_content_renderer_client.h"
#endif

namespace atom {

namespace {

const char* kRelauncherProcess = "relauncher";

bool IsBrowserProcess(base::CommandLine* cmd) {
  std::string process_type = cmd->GetSwitchValueASCII(::switches::kProcessType);
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
  if (!command_line->HasSwitch(::switches::kEnableLogging) &&
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

void LoadExtensionResources() {
  // create a shared resource bundle if one does not already exist
  if (!ui::ResourceBundle::HasSharedInstance()) {
    std::string locale = base::CommandLine::ForCurrentProcess()->
                              GetSwitchValueASCII(::switches::kLang);
    ui::ResourceBundle::InitSharedInstanceWithLocale(
      locale, nullptr, ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);
  }

  std::vector<base::FilePath> pak_resource_paths;
#if defined(OS_MACOSX)
  pak_resource_paths.push_back(
            GetResourcesPakFilePathByName("extensions_resources"));
  pak_resource_paths.push_back(
            GetResourcesPakFilePathByName("extensions_renderer_resources"));
  pak_resource_paths.push_back(
            GetResourcesPakFilePathByName("atom_resources"));
  pak_resource_paths.push_back(
            GetResourcesPakFilePathByName("extensions_api_resources"));
#else
  base::FilePath pak_dir;
  PathService::Get(base::DIR_MODULE, &pak_dir);

  // Append returns a new FilePath
  pak_resource_paths.push_back(
    pak_dir.Append(FILE_PATH_LITERAL("extensions_resources.pak")));
  pak_resource_paths.push_back(
    pak_dir.Append(FILE_PATH_LITERAL("extensions_renderer_resources.pak")));
  pak_resource_paths.push_back(
    pak_dir.Append(FILE_PATH_LITERAL("atom_resources.pak")));
  pak_resource_paths.push_back(
    pak_dir.Append(FILE_PATH_LITERAL("extensions_api_resources.pak")));
#endif

  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();

  for (std::vector<base::FilePath>::const_iterator
      path = pak_resource_paths.begin();
      path != pak_resource_paths.end();
      ++path) {
    bundle.AddDataPackFromPath(*path, ui::GetSupportedScaleFactors()[0]);
  }
}

void AtomMainDelegate::PreSandboxStartup() {
  brightray::MainDelegate::PreSandboxStartup();

#if defined(ENABLE_EXTENSIONS)
  LoadExtensionResources();
#endif
  // Set google API key.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (!env->HasVar("GOOGLE_API_ENDPOINT"))
    env->SetVar("GOOGLE_API_ENDPOINT", GOOGLEAPIS_ENDPOINT);
  if (!env->HasVar("GOOGLE_API_KEY"))
    env->SetVar("GOOGLE_API_KEY", GOOGLEAPIS_API_KEY);

  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line->GetSwitchValueASCII(
      ::switches::kProcessType);

  // Only append arguments for browser process.
  if (!IsBrowserProcess(command_line))
    return;

#if defined(OS_LINUX)
  // always disable the sandbox on linux for now
  // https://github.com/brave/browser-laptop/issues/715
  command_line->AppendSwitch(::switches::kNoSandbox);
#endif

  // Allow file:// URIs to read other file:// URIs by default.
  command_line->AppendSwitch(::switches::kAllowFileAccessFromFiles);

#if defined(OS_MACOSX)
  // Enable AVFoundation.
  command_line->AppendSwitch("enable-avfoundation");
#endif
}

content::ContentBrowserClient* AtomMainDelegate::CreateContentBrowserClient() {
#if defined(ENABLE_EXTENSIONS)
  browser_client_.reset(new brave::BraveContentBrowserClient);
#else
  renderer_client_.reset(new AtomBrowserClient);
#endif
  return browser_client_.get();
}

content::ContentRendererClient*
    AtomMainDelegate::CreateContentRendererClient() {
#if defined(ENABLE_EXTENSIONS)
  renderer_client_.reset(new brave::BraveContentRendererClient);
#else
  renderer_client_.reset(new AtomRendererClient);
#endif
  return renderer_client_.get();
}

content::ContentUtilityClient* AtomMainDelegate::CreateContentUtilityClient() {
  utility_client_.reset(new AtomContentUtilityClient);
  return utility_client_.get();
}

int AtomMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  if (process_type == kRelauncherProcess)
    return relauncher::RelauncherMain(main_function_params);
  else
    return -1;
}

#if defined(OS_MACOSX)
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
