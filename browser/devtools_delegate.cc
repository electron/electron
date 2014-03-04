// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/devtools_delegate.h"

#include "base/message_loop/message_loop.h"
#include "base/values.h"
#include "browser/native_window.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_client_host.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/devtools_manager.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/point.h"

namespace atom {

DevToolsDelegate::DevToolsDelegate(NativeWindow* window,
                                   content::WebContents* target_web_contents)
    : content::WebContentsObserver(window->GetWebContents()),
      owner_window_(window),
      embedder_message_dispatcher_(
          new DevToolsEmbedderMessageDispatcher(this)) {
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
  window->AddObserver(this);
  web_contents->GetController().LoadURL(
      GURL("chrome-devtools://devtools/devtools.html?dockSide=undocked"),
      content::Referrer(),
      content::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

DevToolsDelegate::~DevToolsDelegate() {
}

void DevToolsDelegate::DispatchOnEmbedder(const std::string& message) {
  embedder_message_dispatcher_->Dispatch(message);
}

void DevToolsDelegate::InspectedContentsClosing() {
  owner_window_->Close();
}

void DevToolsDelegate::AboutToNavigateRenderView(
      content::RenderViewHost* render_view_host) {
  content::DevToolsClientHost::SetupDevToolsFrontendClient(
      owner_window_->GetWebContents()->GetRenderViewHost());
}

void DevToolsDelegate::OnWindowClosed() {
  base::MessageLoop::current()->DeleteSoon(FROM_HERE, owner_window_);
}

void DevToolsDelegate::ActivateWindow() {
}

void DevToolsDelegate::CloseWindow() {
  owner_window_->Close();
}

void DevToolsDelegate::MoveWindow(int x, int y) {
  owner_window_->SetPosition(gfx::Point(x, y));
}

void DevToolsDelegate::SetDockSide(const std::string& dock_side) {
  owner_window_->Close();
}

void DevToolsDelegate::OpenInNewTab(const std::string& url) {
}

void DevToolsDelegate::SaveToFile(
    const std::string& url, const std::string& content, bool save_as) {
}

void DevToolsDelegate::AppendToFile(
    const std::string& url, const std::string& content) {
}

void DevToolsDelegate::RequestFileSystems() {
}

void DevToolsDelegate::AddFileSystem() {
}

void DevToolsDelegate::RemoveFileSystem(const std::string& file_system_path) {
}

void DevToolsDelegate::IndexPath(
    int request_id, const std::string& file_system_path) {
}

void DevToolsDelegate::StopIndexing(int request_id) {
}

void DevToolsDelegate::SearchInPath(
    int request_id,
    const std::string& file_system_path,
    const std::string& query) {
}

}  // namespace atom
