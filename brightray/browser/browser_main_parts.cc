// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/browser_main_parts.h"

#include "browser/browser_context.h"
#include "browser/devtools_manager_delegate.h"
#include "browser/web_ui_controller_factory.h"
#include "common/main_delegate.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/devtools_http_handler/devtools_http_handler.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "media/base/media_resources.h"
#include "net/proxy/proxy_resolver_v8.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(USE_AURA)
#include "ui/gfx/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "browser/views/views_delegate.h"
#endif

#if defined(USE_X11)
#include "base/environment.h"
#include "base/path_service.h"
#include "base/nix/xdg_util.h"
#include "base/thread_task_runner_handle.h"
#include "browser/brightray_paths.h"
#include "chrome/browser/ui/libgtk2ui/gtk2_ui.h"
#include "ui/base/x/x11_util.h"
#include "ui/base/x/x11_util_internal.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/wm/core/wm_state.h"
#endif

#if defined(OS_WIN)
#include "ui/base/cursor/cursor_loader_win.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/gfx/platform_font_win.h"
#endif

using content::BrowserThread;

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
  if (PathService::Get(DIR_APP_DATA, &path))
    return;
  scoped_ptr<base::Environment> env(base::Environment::Create());
  path = base::nix::GetXDGDirectory(env.get(),
                                    base::nix::kXdgConfigHomeEnvVar,
                                    base::nix::kDotConfigDir);
  PathService::Override(DIR_APP_DATA, path);
}

int BrowserX11ErrorHandler(Display* d, XErrorEvent* error) {
  if (!g_in_x11_io_error_handler) {
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
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
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
      FROM_HERE, base::MessageLoop::QuitWhenIdleClosure());

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

BrowserMainParts::BrowserMainParts() {
}

BrowserMainParts::~BrowserMainParts() {
}

void BrowserMainParts::PreEarlyInitialization() {
#if defined(USE_X11)
  views::LinuxUI::SetInstance(BuildGtk2UI());
  OverrideLinuxAppDataPath();

  // Installs the X11 error handlers for the browser process used during
  // startup. They simply print error messages and exit because
  // we can't shutdown properly while creating and initializing services.
  ui::SetX11ErrorHandlers(nullptr, nullptr);
#endif
}

void BrowserMainParts::ToolkitInitialized() {
#if defined(USE_AURA) && defined(USE_X11)
  views::LinuxUI::instance()->Initialize();
  wm_state_.reset(new wm::WMState);
#endif

#if defined(TOOLKIT_VIEWS)
  views_delegate_.reset(new ViewsDelegate);
#endif

#if defined(OS_WIN)
  gfx::PlatformFontWin::adjust_font_callback = &AdjustUIFont;
  gfx::PlatformFontWin::get_minimum_font_size_callback = &GetMinimumFontSize;

  wchar_t module_name[MAX_PATH] = { 0 };
  if (GetModuleFileName(NULL, module_name, MAX_PATH))
    ui::CursorLoaderWin::SetCursorResourceModule(module_name);
#endif
}

void BrowserMainParts::PreMainMessageLoopStart() {
#if defined(OS_MACOSX)
  l10n_util::OverrideLocaleWithCocoaLocale();
#endif
  InitializeResourceBundle("");
#if defined(OS_MACOSX)
  InitializeMainNib();
#endif
  media::SetLocalizedStringProvider(MediaStringProvider);
}

void BrowserMainParts::PreMainMessageLoopRun() {
  browser_context_ = BrowserContext::From("", false);

  content::WebUIControllerFactory::RegisterFactory(
      WebUIControllerFactory::GetInstance());

  // --remote-debugging-port
  auto command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kRemoteDebuggingPort))
    devtools_http_handler_.reset(DevToolsManagerDelegate::CreateHttpHandler());
}

void BrowserMainParts::PostMainMessageLoopStart() {
#if defined(USE_X11)
  // Installs the X11 error handlers for the browser process after the
  // main message loop has started. This will allow us to exit cleanly
  // if X exits before us.
  ui::SetX11ErrorHandlers(BrowserX11ErrorHandler, BrowserX11IOErrorHandler);
#endif
}

void BrowserMainParts::PostMainMessageLoopRun() {
  browser_context_ = nullptr;

#if defined(USE_X11)
  // Unset the X11 error handlers. The X11 error handlers log the errors using a
  // |PostTask()| on the message-loop. But since the message-loop is in the
  // process of terminating, this can cause errors.
  ui::SetX11ErrorHandlers(X11EmptyErrorHandler, X11EmptyIOErrorHandler);
#endif
}

int BrowserMainParts::PreCreateThreads() {
#if defined(USE_AURA)
  gfx::Screen* screen = views::CreateDesktopScreen();
  gfx::Screen::SetScreenInstance(screen);
#if defined(USE_X11)
  views::LinuxUI::instance()->UpdateDeviceScaleFactor(
      screen->GetPrimaryDisplay().device_scale_factor());
#endif
#endif
  return 0;
}

}  // namespace brightray
