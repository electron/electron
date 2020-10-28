// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/electron_main_delegate.h"

#include <iostream>
#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "chrome/common/chrome_paths.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "content/public/common/content_switches.h"
#include "electron/buildflags/buildflags.h"
#include "extensions/common/constants.h"
#include "ipc/ipc_buildflags.h"
#include "sandbox/policy/switches.h"
#include "services/tracing/public/cpp/stack_sampling/tracing_sampler_profiler.h"
#include "shell/app/electron_content_client.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_gpu_client.h"
#include "shell/browser/feature_list.h"
#include "shell/browser/relauncher.h"
#include "shell/common/electron_paths.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/electron_renderer_client.h"
#include "shell/renderer/electron_sandboxed_renderer_client.h"
#include "shell/utility/electron_content_utility_client.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_switches.h"

#if defined(OS_MAC)
#include "shell/app/electron_main_delegate_mac.h"
#endif

#if defined(OS_WIN)
#include "base/win/win_util.h"
#include "chrome/child/v8_crashpad_support_win.h"
#endif

#if defined(OS_LINUX)
#include "components/crash/core/app/breakpad_linux.h"
#include "v8/include/v8-wasm-trap-handler-posix.h"
#include "v8/include/v8.h"
#endif

#if !defined(MAS_BUILD)
#include "components/crash/core/app/crashpad.h"  // nogncheck
#include "components/crash/core/common/crash_key.h"
#include "components/crash/core/common/crash_keys.h"
#include "shell/app/electron_crash_reporter_client.h"
#include "shell/browser/api/electron_api_crash_reporter.h"
#include "shell/common/crash_keys.h"
#endif

namespace electron {

namespace {

const char* kRelauncherProcess = "relauncher";

bool IsBrowserProcess(base::CommandLine* cmd) {
  std::string process_type = cmd->GetSwitchValueASCII(::switches::kProcessType);
  return process_type.empty();
}

bool IsSandboxEnabled(base::CommandLine* command_line) {
  return command_line->HasSwitch(switches::kEnableSandbox) ||
         !command_line->HasSwitch(sandbox::policy::switches::kNoSandbox);
}

// Returns true if this subprocess type needs the ResourceBundle initialized
// and resources loaded.
bool SubprocessNeedsResourceBundle(const std::string& process_type) {
  return
#if defined(OS_LINUX)
      // The zygote process opens the resources for the renderers.
      process_type == ::switches::kZygoteProcess ||
#endif
#if defined(OS_MAC)
      // Mac needs them too for scrollbar related images and for sandbox
      // profiles.
      process_type == ::switches::kPpapiPluginProcess ||
      process_type == ::switches::kGpuProcess ||
#endif
      process_type == ::switches::kRendererProcess ||
      process_type == ::switches::kUtilityProcess;
}

#if defined(OS_WIN)
void InvalidParameterHandler(const wchar_t*,
                             const wchar_t*,
                             const wchar_t*,
                             unsigned int,
                             uintptr_t) {
  // noop.
}
#endif

// TODO(nornagon): move path provider overriding to its own file in
// shell/common
bool GetDefaultCrashDumpsPath(base::FilePath* path) {
  base::FilePath cur;
  if (!base::PathService::Get(DIR_USER_DATA, &cur))
    return false;
#if defined(OS_MAC) || defined(OS_WIN)
  cur = cur.Append(FILE_PATH_LITERAL("Crashpad"));
#else
  cur = cur.Append(FILE_PATH_LITERAL("Crash Reports"));
#endif
  // TODO(bauerb): http://crbug.com/259796
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (!base::PathExists(cur) && !base::CreateDirectory(cur))
    return false;
  *path = cur;
  return true;
}

bool ElectronPathProvider(int key, base::FilePath* path) {
  if (key == DIR_CRASH_DUMPS) {
    return GetDefaultCrashDumpsPath(path);
  }
  return false;
}

void RegisterPathProvider() {
  base::PathService::RegisterProvider(ElectronPathProvider, PATH_START,
                                      PATH_END);
}

}  // namespace

std::string LoadResourceBundle(const std::string& locale) {
  const bool initialized = ui::ResourceBundle::HasSharedInstance();
  DCHECK(!initialized);

  // Load other resource files.
  base::FilePath pak_dir;
#if defined(OS_MAC)
  pak_dir =
      base::mac::FrameworkBundlePath().Append(FILE_PATH_LITERAL("Resources"));
#else
  base::PathService::Get(base::DIR_MODULE, &pak_dir);
#endif

  std::string loaded_locale = ui::ResourceBundle::InitSharedInstanceWithLocale(
      locale, nullptr, ui::ResourceBundle::LOAD_COMMON_RESOURCES);
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  bundle.AddDataPackFromPath(pak_dir.Append(FILE_PATH_LITERAL("resources.pak")),
                             ui::SCALE_FACTOR_NONE);
  return loaded_locale;
}

ElectronMainDelegate::ElectronMainDelegate() = default;

ElectronMainDelegate::~ElectronMainDelegate() = default;

const char* const ElectronMainDelegate::kNonWildcardDomainNonPortSchemes[] = {
    extensions::kExtensionScheme};
const size_t ElectronMainDelegate::kNonWildcardDomainNonPortSchemesSize =
    base::size(kNonWildcardDomainNonPortSchemes);

bool ElectronMainDelegate::BasicStartupComplete(int* exit_code) {
  auto* command_line = base::CommandLine::ForCurrentProcess();

  logging::LoggingSettings settings;
#if defined(OS_WIN)
  v8_crashpad_support::SetUp();

  // On Windows the terminal returns immediately, so we add a new line to
  // prevent output in the same line as the prompt.
  if (IsBrowserProcess(command_line))
    std::wcout << std::endl;
#if defined(DEBUG)
  // Print logging to debug.log on Windows
  settings.logging_dest = logging::LOG_TO_ALL;
  base::FilePath log_filename;
  base::PathService::Get(base::DIR_EXE, &log_filename);
  log_filename = log_filename.AppendASCII("debug.log");
  settings.log_file_path = log_filename.value().c_str();
  settings.lock_log = logging::LOCK_LOG_FILE;
  settings.delete_old = logging::DELETE_OLD_LOG_FILE;
#else
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
#endif  // defined(DEBUG)
#else   // defined(OS_WIN)
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
#endif  // !defined(OS_WIN)

  // Only enable logging when --enable-logging is specified.
  auto env = base::Environment::Create();
  if (!command_line->HasSwitch(::switches::kEnableLogging) &&
      !env->HasVar("ELECTRON_ENABLE_LOGGING")) {
    settings.logging_dest = logging::LOG_NONE;
    logging::SetMinLogLevel(logging::LOGGING_NUM_SEVERITIES);
  }

  logging::InitLogging(settings);

  // Logging with pid and timestamp.
  logging::SetLogItems(true, false, true, false);

  // Enable convenient stack printing. This is enabled by default in
  // non-official builds.
  if (env->HasVar("ELECTRON_ENABLE_STACK_DUMPING"))
    base::debug::EnableInProcessStackDumping();

  if (env->HasVar("ELECTRON_DISABLE_SANDBOX"))
    command_line->AppendSwitch(sandbox::policy::switches::kNoSandbox);

  tracing_sampler_profiler_ =
      tracing::TracingSamplerProfiler::CreateOnMainThread();

  chrome::RegisterPathProvider();
  electron::RegisterPathProvider();

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  ContentSettingsPattern::SetNonWildcardDomainNonPortSchemes(
      kNonWildcardDomainNonPortSchemes, kNonWildcardDomainNonPortSchemesSize);
#endif

#if defined(OS_MAC)
  OverrideChildProcessPath();
  OverrideFrameworkBundlePath();
  SetUpBundleOverrides();
#endif

#if defined(OS_WIN)
  // Ignore invalid parameter errors.
  _set_invalid_parameter_handler(InvalidParameterHandler);
  // Disable the ActiveVerifier, which is used by Chrome to track possible
  // bugs, but no use in Electron.
  base::win::DisableHandleVerifier();

  if (IsBrowserProcess(command_line))
    base::win::PinUser32();
#endif

#if defined(OS_LINUX)
  // Check for --no-sandbox parameter when running as root.
  if (getuid() == 0 && IsSandboxEnabled(command_line))
    LOG(FATAL) << "Running as root without --"
               << sandbox::policy::switches::kNoSandbox
               << " is not supported. See https://crbug.com/638180.";
#endif

#if defined(MAS_BUILD)
  // In MAS build we are using --disable-remote-core-animation.
  //
  // According to ccameron:
  // If you're running with --disable-remote-core-animation, you may want to
  // also run with --disable-gpu-memory-buffer-compositor-resources as well.
  // That flag makes it so we use regular GL textures instead of IOSurfaces
  // for compositor resources. IOSurfaces are very heavyweight to
  // create/destroy, but they can be displayed directly by CoreAnimation (and
  // --disable-remote-core-animation makes it so we don't use this property,
  // so they're just heavyweight with no upside).
  command_line->AppendSwitch(
      ::switches::kDisableGpuMemoryBufferCompositorResources);
#endif

  content_client_ = std::make_unique<ElectronContentClient>();
  SetContentClient(content_client_.get());

  return false;
}

void ElectronMainDelegate::PreSandboxStartup() {
  auto* command_line = base::CommandLine::ForCurrentProcess();

  std::string process_type =
      command_line->GetSwitchValueASCII(::switches::kProcessType);

#if !defined(MAS_BUILD)
  crash_reporter::InitializeCrashKeys();
#endif

  // Initialize ResourceBundle which handles files loaded from external
  // sources. The language should have been passed in to us from the
  // browser process as a command line flag.
  if (SubprocessNeedsResourceBundle(process_type)) {
    std::string locale = command_line->GetSwitchValueASCII(::switches::kLang);
    LoadResourceBundle(locale);
  }

#if defined(OS_WIN) || (defined(OS_MAC) && !defined(MAS_BUILD))
  // In the main process, we wait for JS to call crashReporter.start() before
  // initializing crashpad. If we're in the renderer, we want to initialize it
  // immediately at boot.
  if (!process_type.empty()) {
    ElectronCrashReporterClient::Create();
    crash_reporter::InitializeCrashpad(false, process_type);
  }
#endif

#if defined(OS_LINUX)
  if (process_type != ::switches::kZygoteProcess && !process_type.empty()) {
    ElectronCrashReporterClient::Create();
    breakpad::InitCrashReporter(process_type);
  }
#endif

#if !defined(MAS_BUILD)
  crash_keys::SetCrashKeysFromCommandLine(*command_line);
  crash_keys::SetPlatformCrashKey();
#endif

  if (IsBrowserProcess(command_line)) {
    // Only append arguments for browser process.

    // Allow file:// URIs to read other file:// URIs by default.
    command_line->AppendSwitch(::switches::kAllowFileAccessFromFiles);

#if defined(OS_MAC)
    // Enable AVFoundation.
    command_line->AppendSwitch("enable-avfoundation");
#endif
  }
}

void ElectronMainDelegate::PreCreateMainMessageLoop() {
  // This is initialized early because the service manager reads some feature
  // flags and we need to make sure the feature list is initialized before the
  // service manager reads the features.
  InitializeFeatureList();
#if defined(OS_MAC)
  RegisterAtomCrApp();
#endif
}

content::ContentBrowserClient*
ElectronMainDelegate::CreateContentBrowserClient() {
  browser_client_ = std::make_unique<ElectronBrowserClient>();
  return browser_client_.get();
}

content::ContentGpuClient* ElectronMainDelegate::CreateContentGpuClient() {
  gpu_client_ = std::make_unique<ElectronGpuClient>();
  return gpu_client_.get();
}

content::ContentRendererClient*
ElectronMainDelegate::CreateContentRendererClient() {
  auto* command_line = base::CommandLine::ForCurrentProcess();

  if (IsSandboxEnabled(command_line)) {
    renderer_client_ = std::make_unique<ElectronSandboxedRendererClient>();
  } else {
    renderer_client_ = std::make_unique<ElectronRendererClient>();
  }

  return renderer_client_.get();
}

content::ContentUtilityClient*
ElectronMainDelegate::CreateContentUtilityClient() {
  utility_client_ = std::make_unique<ElectronContentUtilityClient>();
  return utility_client_.get();
}

int ElectronMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  if (process_type == kRelauncherProcess)
    return relauncher::RelauncherMain(main_function_params);
  else
    return -1;
}

bool ElectronMainDelegate::ShouldCreateFeatureList() {
  return false;
}

bool ElectronMainDelegate::ShouldLockSchemeRegistry() {
  return false;
}

#if defined(OS_LINUX)
void ElectronMainDelegate::ZygoteForked() {
  // Needs to be called after we have DIR_USER_DATA.  BrowserMain sets
  // this up for the browser process in a different manner.
  ElectronCrashReporterClient::Create();
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(::switches::kProcessType);
  breakpad::InitCrashReporter(process_type);

  // Reset the command line for the newly spawned process.
  crash_keys::SetCrashKeysFromCommandLine(*command_line);
}
#endif  // defined(OS_LINUX)

}  // namespace electron
