// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_main_parts.h"

#include "browser/api/atom_bindings.h"
#include "browser/atom_browser_context.h"
#include "browser/native_window.h"
#include "common/node_bindings.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

AtomBrowserMainParts::AtomBrowserMainParts()
    : atom_bindings_(new AtomBindings),
      node_bindings_(NodeBindings::Create(true)) {
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
}

brightray::BrowserContext* AtomBrowserMainParts::CreateBrowserContext() {
  return new AtomBrowserContext();
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  brightray::BrowserMainParts::PostEarlyInitialization();

  node_bindings_->Initialize();

  atom_bindings_->BindTo(node::process);

  node_bindings_->Load();

  atom_bindings_->AfterLoad();
}

void AtomBrowserMainParts::PreMainMessageLoopStart() {
  brightray::BrowserMainParts::PreMainMessageLoopStart();

  node_bindings_->PrepareMessageLoop();
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
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
}

}  // namespace atom
