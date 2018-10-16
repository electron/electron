// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/browser_main_parts.h"

#if defined(OS_POSIX)
#include <stdlib.h>
#endif

#include <sys/stat.h>
#include <string>
#include <utility>

#if defined(OS_LINUX)
#include <glib.h>  // for g_setenv()
#endif

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/browser_client.h"
#include "brightray/browser/devtools_manager_delegate.h"
#include "brightray/browser/media/media_capture_devices_dispatcher.h"
#include "brightray/browser/web_ui_controller_factory.h"
#include "brightray/common/application_info.h"
#include "brightray/common/main_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "media/base/localized_strings.h"
#include "net/proxy_resolution/proxy_resolver_v8.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_switches.h"

#if defined(USE_AURA)
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/wm/core/wm_state.h"
#endif

#if defined(USE_X11)
#include "base/environment.h"
#include "base/nix/xdg_util.h"
#include "base/path_service.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brightray/browser/brightray_paths.h"
#include "chrome/browser/ui/libgtkui/gtk_ui.h"
#include "ui/base/x/x11_util.h"
#include "ui/base/x/x11_util_internal.h"
#include "ui/views/linux_ui/linux_ui.h"
#endif

#if defined(OS_WIN)
#include "ui/base/cursor/cursor_loader_win.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/gfx/platform_font_win.h"
#endif

#if defined(OS_LINUX)
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/dbus/dbus_bluez_manager_wrapper_linux.h"
#endif

namespace brightray {

namespace {

#if defined(OS_WIN)
// gfx::Font callbacks
void AdjustUIFont(LOGFONT* logfont) {
  l10n_util::AdjustUIFont(logfont);
}

int GetMinimumFontSize() {
  return 10;
}
#endif

#if defined(USE_X11)
// Indicates that we're currently responding to an IO error (by shutting down).
bool g_in_x11_io_error_handler = false;

// Number of seconds to wait for UI thread to get an IO error if we get it on
// the background thread.
const int kWaitForUIThreadSeconds = 10;

void OverrideLinuxAppDataPath() {
  base::FilePath path;
  if (base::PathService::Get(DIR_APP_DATA, &path))
    return;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  path = base::nix::GetXDGDirectory(env.get(), base::nix::kXdgConfigHomeEnvVar,
                                    base::nix::kDotConfigDir);
  base::PathService::Override(DIR_APP_DATA, path);
}

int BrowserX11ErrorHandler(Display* d, XErrorEvent* error) {
  if (!g_in_x11_io_error_handler && base::ThreadTaskRunnerHandle::IsSet()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ui::LogErrorEventDescription, d, *error));
  }
  return 0;
}

// This function is used to help us diagnose crash dumps that happen
// during the shutdown process.
NOINLINE void WaitingForUIThreadToHandleIOError() {
  // Ensure function isn't optimized away.
  asm("");
  sleep(kWaitForUIThreadSeconds);
}

int BrowserX11IOErrorHandler(Display* d) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    // Wait for the UI thread (which has a different connection to the X server)
    // to get the error. We can't call shutdown from this thread without
    // tripping an error. Doing it through a function so that we'll be able
    // to see it in any crash dumps.
    WaitingForUIThreadToHandleIOError();
    return 0;
  }

  // If there's an IO error it likely means the X server has gone away.
  // If this CHECK fails, then that means SessionEnding() below triggered some
  // code that tried to talk to the X server, resulting in yet another error.
  CHECK(!g_in_x11_io_error_handler);

  g_in_x11_io_error_handler = true;
  LOG(ERROR) << "X IO error received (X server probably went away)";
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());

  return 0;
}

int X11EmptyErrorHandler(Display* d, XErrorEvent* error) {
  return 0;
}

int X11EmptyIOErrorHandler(Display* d) {
  return 0;
}
#endif

base::string16 MediaStringProvider(media::MessageId id) {
  switch (id) {
    case media::DEFAULT_AUDIO_DEVICE_NAME:
      return base::ASCIIToUTF16("Default");
#if defined(OS_WIN)
    case media::COMMUNICATIONS_AUDIO_DEVICE_NAME:
      return base::ASCIIToUTF16("Communications");
#endif
    default:
      return base::string16();
  }
}

}  // namespace

BrowserMainParts::BrowserMainParts() {}

BrowserMainParts::~BrowserMainParts() {}

#if defined(OS_WIN) || defined(OS_LINUX)
void OverrideAppLogsPath() {
  base::FilePath path;
  if (base::PathService::Get(brightray::DIR_APP_DATA, &path)) {
    path = path.Append(base::FilePath::FromUTF8Unsafe(GetApplicationName()));
    path = path.Append(base::FilePath::FromUTF8Unsafe("logs"));
    base::PathService::Override(DIR_APP_LOGS, path);
  }
}
#endif

void BrowserMainParts::InitializeFeatureList() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  auto enable_features =
      cmd_line->GetSwitchValueASCII(switches::kEnableFeatures);
  // Node depends on SharedArrayBuffer support, which was temporarily disabled
  // by https://chromium-review.googlesource.com/c/chromium/src/+/849429 (in
  // M64) and reenabled by
  // https://chromium-review.googlesource.com/c/chromium/src/+/1159358 (in
  // M70). Once Electron upgrades to M70, we can remove this.
  enable_features += std::string(",") + features::kSharedArrayBuffer.name;
  auto disable_features =
      cmd_line->GetSwitchValueASCII(switches::kDisableFeatures);
#if defined(OS_MACOSX)
  // Disable the V2 sandbox on macOS.
  // Chromium is going to use the system sandbox API of macOS for the sandbox
  // implmentation, we may have to deprecate --mixed-sandbox for macOS once
  // Chromium drops support for the old sandbox implmentation.
  disable_features += std::string(",") + features::kMacV2Sandbox.name;
#endif
  auto feature_list = std::make_unique<base::FeatureList>();
  feature_list->InitializeFromCommandLine(enable_features, disable_features);
  base::FeatureList::SetInstance(std::move(feature_list));
}

bool BrowserMainParts::ShouldContentCreateFeatureList() {
  return false;
}

int BrowserMainParts::PreEarlyInitialization() {
  InitializeFeatureList();
  OverrideAppLogsPath();
#if defined(USE_X11)
  views::LinuxUI::SetInstance(BuildGtkUi());
  OverrideLinuxAppDataPath();

  // Installs the X11 error handlers for the browser process used during
  // startup. They simply print error messages and exit because
  // we can't shutdown properly while creating and initializing services.
  ui::SetX11ErrorHandlers(nullptr, nullptr);
#endif

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void BrowserMainParts::ToolkitInitialized() {
  ui::MaterialDesignController::Initialize();

#if defined(USE_AURA) && defined(USE_X11)
  views::LinuxUI::instance()->Initialize();
#endif

#if defined(USE_AURA)
  wm_state_.reset(new wm::WMState);
#endif

#if defined(OS_WIN)
  gfx::PlatformFontWin::adjust_font_callback = &AdjustUIFont;
  gfx::PlatformFontWin::get_minimum_font_size_callback = &GetMinimumFontSize;

  wchar_t module_name[MAX_PATH] = {0};
  if (GetModuleFileName(NULL, module_name, MAX_PATH))
    ui::CursorLoaderWin::SetCursorResourceModule(module_name);
#endif
}

void BrowserMainParts::PreMainMessageLoopStart() {
  // Initialize ui::ResourceBundle.
  ui::ResourceBundle::InitSharedInstanceWithLocale(
      "", nullptr, ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kLang)) {
    const std::string locale = cmd_line->GetSwitchValueASCII(switches::kLang);
    const base::FilePath locale_file_path =
        ui::ResourceBundle::GetSharedInstance().GetLocaleFilePath(locale, true);
    if (!locale_file_path.empty()) {
      custom_locale_ = locale;
#if defined(OS_LINUX)
      /* When built with USE_GLIB, libcc's GetApplicationLocaleInternal() uses
       * glib's g_get_language_names(), which keys off of getenv("LC_ALL") */
      g_setenv("LC_ALL", custom_locale_.c_str(), TRUE);
#endif
    }
  }

#if defined(OS_MACOSX)
  if (custom_locale_.empty())
    l10n_util::OverrideLocaleWithCocoaLocale();
#endif
  LoadResourceBundle(custom_locale_);
#if defined(OS_MACOSX)
  InitializeMainNib();
#endif
  media::SetLocalizedStringProvider(MediaStringProvider);
}

void BrowserMainParts::PreMainMessageLoopRun() {
  content::WebUIControllerFactory::RegisterFactory(
      WebUIControllerFactory::GetInstance());

  // --remote-debugging-port
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kRemoteDebuggingPort))
    DevToolsManagerDelegate::StartHttpHandler();
}

void BrowserMainParts::PostMainMessageLoopStart() {
#if defined(USE_X11)
  // Installs the X11 error handlers for the browser process after the
  // main message loop has started. This will allow us to exit cleanly
  // if X exits before us.
  ui::SetX11ErrorHandlers(BrowserX11ErrorHandler, BrowserX11IOErrorHandler);
#endif
#if defined(OS_LINUX)
  bluez::DBusBluezManagerWrapperLinux::Initialize();
#endif
}

void BrowserMainParts::PostMainMessageLoopRun() {
#if defined(USE_X11)
  // Unset the X11 error handlers. The X11 error handlers log the errors using a
  // |PostTask()| on the message-loop. But since the message-loop is in the
  // process of terminating, this can cause errors.
  ui::SetX11ErrorHandlers(X11EmptyErrorHandler, X11EmptyIOErrorHandler);
#endif
}

int BrowserMainParts::PreCreateThreads() {
#if defined(USE_AURA)
  display::Screen* screen = views::CreateDesktopScreen();
  display::Screen::SetScreenInstance(screen);
#if defined(USE_X11)
  views::LinuxUI::instance()->UpdateDeviceScaleFactor();
#endif
#endif

  // Force MediaCaptureDevicesDispatcher to be created on UI thread.
  MediaCaptureDevicesDispatcher::GetInstance();

  if (!views::LayoutProvider::Get())
    layout_provider_.reset(new views::LayoutProvider());

  // Initialize the app locale.
  BrowserClient::SetApplicationLocale(
      l10n_util::GetApplicationLocale(custom_locale_));

  return 0;
}

void BrowserMainParts::PostDestroyThreads() {
#if defined(OS_LINUX)
  device::BluetoothAdapterFactory::Shutdown();
  bluez::DBusBluezManagerWrapperLinux::Shutdown();
#endif
}

}  // namespace brightray
