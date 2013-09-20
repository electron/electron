// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_main_parts.h"

#include "base/power_monitor/power_monitor.h"
#include "browser/api/atom_browser_bindings.h"
#include "browser/atom_browser_client.h"
#include "browser/atom_browser_context.h"
#include "browser/browser.h"
#include "common/node_bindings.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

// static
AtomBrowserMainParts* AtomBrowserMainParts::self_ = NULL;

AtomBrowserMainParts::AtomBrowserMainParts()
    : atom_bindings_(new AtomBrowserBindings),
      browser_(new Browser),
      node_bindings_(NodeBindings::Create(true)) {
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

#if defined(OS_MACOSX)
  base::PowerMonitor::AllocateSystemIOPorts();
#endif

  node_bindings_->Initialize();

  // Wrap whole process in one global context.
  node::g_context->Enter();

  atom_bindings_->BindTo(node::process);

  node_bindings_->Load();

  atom_bindings_->AfterLoad();
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  node_bindings_->PrepareMessageLoop();

  brightray::BrowserMainParts::PreMainMessageLoopRun();

  {
    v8::HandleScope scope;
    v8::Context::Scope context_scope(node::g_context);

    v8::Handle<v8::Value> args;
    node::MakeCallback(atom_bindings_->browser_main_parts(),
                       "preMainMessageLoopRun",
                       0, &args);
  }

  node_bindings_->RunMessageLoop();

  // Make sure the url request job factory is created before the
  // will-finish-launching event.
  static_cast<content::BrowserContext*>(AtomBrowserContext::Get())->
      GetRequestContext();

#if !defined(OS_MACOSX)
  // The corresponding call in OS X is in AtomApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  Browser::Get()->DidFinishLaunching();
#endif
}

}  // namespace atom
