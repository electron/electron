// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_main_parts.h"

#include "base/values.h"
#include "browser/native_window.h"
#include "brightray/browser/browser_context.h"
#include "brightray/browser/default_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "common/node_bindings.h"

namespace atom {

AtomBrowserMainParts::AtomBrowserMainParts()
    : node_bindings_(new NodeBindings) {
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  node_bindings_->Initialize();
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  brightray::BrowserMainParts::PreMainMessageLoopRun();

  scoped_ptr<base::DictionaryValue> options(new base::DictionaryValue);
  options->SetInteger("width", 800);
  options->SetInteger("height", 600);
  options->SetString("title", "Atom");

  // FIXME: Leak object here.
  NativeWindow* window = NativeWindow::Create(browser_context(), options.get());
  window->InitFromOptions(options.get());

  window->GetWebContents()->GetController().LoadURL(
      GURL("http://adam.roben.org/brightray_example/start.html"),
      content::Referrer(),
      content::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

}  // namespace atom
