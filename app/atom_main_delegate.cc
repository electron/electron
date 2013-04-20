// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/atom_main_delegate.h"

#include "base/command_line.h"
#include "browser/atom_browser_client.h"
#include "content/public/common/content_switches.h"
#include "renderer/atom_renderer_client.h"

namespace atom {

AtomMainDelegate::AtomMainDelegate() {
}

AtomMainDelegate::~AtomMainDelegate() {
}

void AtomMainDelegate::PreSandboxStartup() {
  brightray::MainDelegate::PreSandboxStartup();

  // Disable renderer sandbox for most of node's functions.
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(switches::kNoSandbox);
}

content::ContentBrowserClient* AtomMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new AtomBrowserClient);
  return browser_client_.get();
}

content::ContentRendererClient*
    AtomMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new AtomRendererClient);
  return renderer_client_.get();
}

}  // namespace atom
