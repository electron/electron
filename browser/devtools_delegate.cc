// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/devtools_delegate.h"

#include "base/values.h"
#include "browser/native_window.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_client_host.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/devtools_manager.h"
#include "content/public/browser/web_contents.h"

namespace atom {

DevToolsDelegate::DevToolsDelegate(NativeWindow* window,
                                   content::WebContents* target_web_contents)
    : content::WebContentsObserver(window->GetWebContents()),
      owner_window_(window) {
  content::WebContents* web_contents = window->GetWebContents();

  // Setup devtools.
  devtools_agent_host_ = content::DevToolsAgentHost::GetOrCreateFor(
      target_web_contents->GetRenderViewHost());
  devtools_client_host_.reset(
      content::DevToolsClientHost::CreateDevToolsFrontendHost(web_contents,
                                                              this));
  content::DevToolsManager::GetInstance()->RegisterDevToolsClientHostFor(
      devtools_agent_host_.get(), devtools_client_host_.get());

  // Go!
  base::DictionaryValue options;
  options.SetString("title", "DevTools Debugger");
  window->InitFromOptions(&options);
  web_contents->GetController().LoadURL(
      GURL("chrome-devtools://devtools/devtools.html"),
      content::Referrer(),
      content::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

DevToolsDelegate::~DevToolsDelegate() {
}

void DevToolsDelegate::DispatchOnEmbedder(const std::string& message) {
}

void DevToolsDelegate::InspectedContentsClosing() {
  delete owner_window_;
}

void DevToolsDelegate::AboutToNavigateRenderView(
      content::RenderViewHost* render_view_host) {
  content::DevToolsClientHost::SetupDevToolsFrontendClient(
      owner_window_->GetWebContents()->GetRenderViewHost());
}

void DevToolsDelegate::OnWindowClosed() {
  delete owner_window_;
}

}  // namespace atom
