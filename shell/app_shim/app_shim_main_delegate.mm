// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app_shim/app_shim_main_delegate.h"

#import <Cocoa/Cocoa.h>

#include <utility>

#include "base/base_switches.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "components/remote_cocoa/app_shim/application_bridge.h"
#include "mojo/core/embedder/configuration.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/system/functions.h"
#include "shell/app/electron_content_client.h"
#include "shell/app/electron_main_delegate.h"
#import "shell/app_shim/app_shim_application.h"
#include "shell/app_shim/app_shim_controller.h"
#include "shell/common/app_shim_constants.h"
#include "ui/accelerated_widget_mac/window_resize_helper_mac.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace electron {

namespace {

void HandleBadMessage(const std::string& error) {
  LOG(ERROR) << "Mojo error in app shim process: " << error;
}

}  // namespace

AppShimMainDelegate::AppShimMainDelegate(const base::FilePath& host_bundle_path)
    : host_bundle_path_(host_bundle_path) {}

AppShimMainDelegate::~AppShimMainDelegate() = default;

std::optional<int> AppShimMainDelegate::BasicStartupComplete() {
  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  logging::InitLogging(settings);

  remote_cocoa::ApplicationBridge::SetIsOutOfProcessAppShim();
  return std::nullopt;
}

void AppShimMainDelegate::PreSandboxStartup() {
  // A shim is started by Launch Services, not by the browser, so it has no
  // field trial state to inherit: features are the defaults, changed only by
  // this process's own command line.
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  base::FeatureList::InitInstance(
      command_line->GetSwitchValueASCII(::switches::kEnableFeatures),
      command_line->GetSwitchValueASCII(::switches::kDisableFeatures));

  // Localized strings and cursors for the views that draw in this process, in
  // the language the host app would pick.
  l10n_util::OverrideLocaleWithCocoaLocale();
  LoadResourceBundle(l10n_util::GetLocaleOverride());

  // Mojo's features must match the browser's to speak the same IPC.
  mojo::core::InitFeatures();
  // The connection to the browser is isolated, so this process is a broker.
  mojo::core::Configuration config;
  config.is_broker_process = true;
  mojo::core::Init(config);
  mojo::SetDefaultProcessErrorHandler(base::BindRepeating(&HandleBadMessage));
}

std::variant<int, content::MainFunctionParams> AppShimMainDelegate::RunProcess(
    const std::string& process_type,
    content::MainFunctionParams main_function_params) {
  if (process_type != app_shim::kProcessType)
    return std::move(main_function_params);

  base::Thread::Options io_thread_options;
  io_thread_options.message_pump_type = base::MessagePumpType::IO;
  // Leaked on purpose: it must outlive every Mojo object in the process.
  base::Thread* io_thread = new base::Thread("ElectronAppShimIO");
  io_thread->StartWithOptions(std::move(io_thread_options));

  mojo::core::ScopedIPCSupport ipc_support(
      io_thread->task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::FAST);

  // The NSApplication must be the shim's own, so must not exist yet.
  [ElectronAppShimApplication sharedApplication];
  CHECK([NSApp isKindOfClass:[ElectronAppShimApplication class]]);

  base::SingleThreadTaskExecutor main_task_executor(base::MessagePumpType::UI,
                                                    /*is_main_thread=*/true);
  ui::WindowResizeHelperMac::Get()->Init(main_task_executor.task_runner());
  // content made the pool for this process type but only starts it for the
  // process types it runs itself.
  base::ThreadPoolInstance::Get()->StartWithDefaultParams();
  base::PlatformThread::SetName("ElectronAppShimMain");

  controller_ = std::make_unique<AppShimController>(host_bundle_path_);

  base::RunLoop().Run();
  return 0;
}

bool AppShimMainDelegate::ShouldCreateFeatureList(InvokedIn invoked_in) {
  // Created in PreSandboxStartup.
  return false;
}

bool AppShimMainDelegate::ShouldInitializeMojo(InvokedIn invoked_in) {
  // Initialized in PreSandboxStartup, as a broker.
  return false;
}

bool AppShimMainDelegate::ShouldInitializePerfetto(InvokedIn invoked_in) {
  return false;
}

bool AppShimMainDelegate::ShouldReconfigurePartitionAlloc() {
  return false;
}

bool AppShimMainDelegate::ShouldLoadV8Snapshot(
    const std::string& process_type) {
  // An app shim runs no JavaScript.
  return false;
}

content::ContentClient* AppShimMainDelegate::CreateContentClient() {
  content_client_ = std::make_unique<ElectronContentClient>();
  return content_client_.get();
}

}  // namespace electron
