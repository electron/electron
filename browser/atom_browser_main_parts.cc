// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_main_parts.h"

#include "base/values.h"
#include "browser/api/atom_bindings.h"
#include "browser/native_window.h"
#include "brightray/browser/browser_context.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "common/node_bindings.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

AtomBrowserMainParts::AtomBrowserMainParts()
    : atom_bindings_(new AtomBindings),
      node_bindings_(NodeBindings::Create(true)) {
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  brightray::BrowserMainParts::PostEarlyInitialization();

  node_bindings_->Initialize();

  atom_bindings_->BindTo(node::process);

  node_bindings_->Load();
}

void AtomBrowserMainParts::PreMainMessageLoopStart() {
  brightray::BrowserMainParts::PreMainMessageLoopStart();

  node_bindings_->PrepareMessageLoop();
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  brightray::BrowserMainParts::PreMainMessageLoopRun();

  node_bindings_->RunMessageLoop();

  scoped_ptr<base::DictionaryValue> options(new base::DictionaryValue);
  options->SetInteger("width", 800);
  options->SetInteger("height", 600);
  options->SetString("title", "Atom");

  // FIXME: Leak object here.
  NativeWindow* window = NativeWindow::Create(browser_context(), options.get());
  window->InitFromOptions(options.get());

  window->GetWebContents()->GetController().LoadURL(
      GURL("http://localhost"),
      content::Referrer(),
      content::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

}  // namespace atom
