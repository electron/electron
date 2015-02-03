// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/browser_main_parts.h"

#include "browser/browser_context.h"
#include "browser/remote_debugging_server.h"
#include "browser/web_ui_controller_factory.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/common/content_switches.h"
#include "net/proxy/proxy_resolver_v8.h"

#if defined(USE_AURA)
#include "ui/gfx/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#endif

#if defined(USE_AURA) && defined(USE_X11)
#include "chrome/browser/ui/libgtk2ui/gtk2_ui.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/wm/core/wm_state.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "browser/views/views_delegate.h"
#endif

#if defined(OS_LINUX)
#include "base/environment.h"
#include "base/path_service.h"
#include "base/nix/xdg_util.h"
#include "browser/brightray_paths.h"
#endif

#if defined(OS_WIN)
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/gfx/platform_font_win.h"
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

#if defined(OS_LINUX)
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
#endif

}  // namespace

BrowserMainParts::BrowserMainParts() {
}

BrowserMainParts::~BrowserMainParts() {
}

void BrowserMainParts::PreEarlyInitialization() {
#if defined(OS_MACOSX)
  IncreaseFileDescriptorLimit();
#endif

#if defined(USE_AURA) && defined(USE_X11)
  views::LinuxUI::SetInstance(BuildGtk2UI());
#endif

#if defined(OS_LINUX)
  OverrideLinuxAppDataPath();
#endif

  InitProxyResolverV8();
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
#endif
}

void BrowserMainParts::PreMainMessageLoopStart() {
#if defined(OS_MACOSX)
  InitializeMainNib();
#endif
}

void BrowserMainParts::PreMainMessageLoopRun() {
  browser_context_.reset(CreateBrowserContext());
  browser_context_->Initialize();

  web_ui_controller_factory_.reset(
      new WebUIControllerFactory(browser_context_.get()));
  content::WebUIControllerFactory::RegisterFactory(
      web_ui_controller_factory_.get());

  // --remote-debugging-port
  base::CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kRemoteDebuggingPort)) {
    std::string port_str = command_line->GetSwitchValueASCII(switches::kRemoteDebuggingPort);
    int port;
    if (base::StringToInt(port_str, &port) && port >= 0 && port < 65535)
      remote_debugging_server_.reset(
          new RemoteDebuggingServer("127.0.0.1", static_cast<uint16>(port)));
    else
      DLOG(WARNING) << "Invalid http debugger port number " << port;
  }
}

void BrowserMainParts::PostMainMessageLoopRun() {
  browser_context_.reset();
}

int BrowserMainParts::PreCreateThreads() {
#if defined(USE_AURA)
  gfx::Screen::SetScreenInstance(gfx::SCREEN_TYPE_NATIVE,
                                 views::CreateDesktopScreen());
#endif

  return 0;
}

BrowserContext* BrowserMainParts::CreateBrowserContext() {
  return new BrowserContext;
}

void BrowserMainParts::InitProxyResolverV8() {
  net::ProxyResolverV8::EnsureIsolateCreated();
}

}  // namespace brightray
