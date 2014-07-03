// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/browser_main_parts.h"

#include "browser/browser_context.h"
#include "browser/web_ui_controller_factory.h"
#include "net/proxy/proxy_resolver_v8.h"
#include "ui/gfx/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"

#if defined(USE_AURA) && defined(USE_X11)
#include "chrome/browser/ui/libgtk2ui/gtk2_ui.h"
#include "ui/views/linux_ui/linux_ui.h"
#endif

namespace brightray {

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
}

void BrowserMainParts::ToolkitInitialized() {
#if defined(USE_AURA) && defined(USE_X11)
  views::LinuxUI::instance()->Initialize();
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

#if defined(OS_WIN)
  gfx::Screen::SetScreenInstance(gfx::SCREEN_TYPE_NATIVE, views::CreateDesktopScreen());
#endif
}

void BrowserMainParts::PostMainMessageLoopRun() {
  browser_context_.reset();
}

int BrowserMainParts::PreCreateThreads() {
#if defined(OS_WIN)
  net::ProxyResolverV8::CreateIsolate();
#else
  net::ProxyResolverV8::RememberDefaultIsolate();
#endif
  return 0;
}

BrowserContext* BrowserMainParts::CreateBrowserContext() {
  return new BrowserContext;
}

}  // namespace brightray
