// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/browser.h"
#include "atom/browser/javascript_environment.h"
#include "atom/browser/node_debugger.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/node_bindings.h"
#include "base/command_line.h"

#if defined(OS_WIN)
#include "ui/gfx/win/dpi.h"
#endif

#if defined(USE_X11)
#include "chrome/browser/ui/libgtk2ui/gtk2_util.h"
#endif

#include "atom/common/node_includes.h"

namespace atom {

// static
AtomBrowserMainParts* AtomBrowserMainParts::self_ = NULL;

AtomBrowserMainParts::AtomBrowserMainParts()
    : browser_(new Browser),
      node_bindings_(NodeBindings::Create(true)),
      atom_bindings_(new AtomBindings) {
  DCHECK(!self_) << "Cannot have two AtomBrowserMainParts";
  self_ = this;
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
}

// static
AtomBrowserMainParts* AtomBrowserMainParts::Get() {
  DCHECK(self_);
  return self_;
}

brightray::BrowserContext* AtomBrowserMainParts::CreateBrowserContext() {
  return new AtomBrowserContext();
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  brightray::BrowserMainParts::PostEarlyInitialization();

  // The ProxyResolverV8 has setup a complete V8 environment, in order to avoid
  // conflicts we only initialize our V8 environment after that.
  js_env_.reset(new JavascriptEnvironment);

  node_bindings_->Initialize();

  // Support the "--debug" switch.
  node_debugger_.reset(new NodeDebugger);

  // Create the global environment.
  global_env = node_bindings_->CreateEnvironment(js_env_->context());

  // Add atom-shell extended APIs.
  atom_bindings_->BindTo(js_env_->isolate(), global_env->process_object());
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  // Run user's main script before most things get initialized, so we can have
  // a chance to setup everything.
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

  brightray::BrowserMainParts::PreMainMessageLoopRun();

#if defined(USE_X11)
  libgtk2ui::GtkInitFromCommandLine(*CommandLine::ForCurrentProcess());
#endif

  // Make sure the url request job factory is created before the ready event.
  static_cast<content::BrowserContext*>(AtomBrowserContext::Get())->
      GetRequestContext();

#if !defined(OS_MACOSX)
  // The corresponding call in OS X is in AtomApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  Browser::Get()->DidFinishLaunching();
#endif
}

}  // namespace atom
