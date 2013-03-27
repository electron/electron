// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/inspectable_web_contents_impl.h"

#include "browser/browser_client.h"
#include "browser/browser_main_parts.h"
#include "browser/inspectable_web_contents_view.h"

#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_client_host.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/devtools_manager.h"
#include "content/public/browser/web_contents_view.h"

namespace brightray {

// Implemented separately on each platform.
InspectableWebContentsView* CreateInspectableContentsView(InspectableWebContentsImpl*);

InspectableWebContentsImpl::InspectableWebContentsImpl(const content::WebContents::CreateParams& create_params)
    : web_contents_(content::WebContents::Create(create_params)) {
  view_.reset(CreateInspectableContentsView(this));
}

InspectableWebContentsImpl::~InspectableWebContentsImpl() {
}

InspectableWebContentsView* InspectableWebContentsImpl::GetView() const {
  return view_.get();
}

content::WebContents* InspectableWebContentsImpl::GetWebContents() const {
  return web_contents_.get();
}

void InspectableWebContentsImpl::ShowDevTools() {
  if (!devtools_web_contents_) {
    devtools_web_contents_.reset(content::WebContents::Create(content::WebContents::CreateParams(web_contents_->GetBrowserContext())));
    Observe(devtools_web_contents_.get());
    devtools_web_contents_->SetDelegate(this);

    agent_host_ = content::DevToolsAgentHost::GetOrCreateFor(web_contents_->GetRenderViewHost());
    frontend_host_.reset(content::DevToolsClientHost::CreateDevToolsFrontendHost(devtools_web_contents_.get(), this));

    auto client = static_cast<BrowserClient*>(content::GetContentClient()->browser());
    auto handler = client->browser_main_parts()->devtools_http_handler();
    auto url = handler->GetFrontendURL(nullptr);
    devtools_web_contents_->GetController().LoadURL(url, content::Referrer(), content::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());
  }

  view_->ShowDevTools();
}

void InspectableWebContentsImpl::ActivateWindow() {
}

void InspectableWebContentsImpl::ChangeAttachedWindowHeight(unsigned height) {
}

void InspectableWebContentsImpl::CloseWindow() {
  view_->CloseDevTools();
  devtools_web_contents_.reset();
  web_contents_->GetView()->Focus();
}

void InspectableWebContentsImpl::MoveWindow(int x, int y) {
}

void InspectableWebContentsImpl::SetDockSide(const std::string& side) {
}

void InspectableWebContentsImpl::OpenInNewTab(const std::string& url) {
}

void InspectableWebContentsImpl::SaveToFile(const std::string& url, const std::string& content, bool save_as) {
}

void InspectableWebContentsImpl::AppendToFile(const std::string& url, const std::string& content) {
}

void InspectableWebContentsImpl::RequestFileSystems() {
}

void InspectableWebContentsImpl::AddFileSystem() {
}

void InspectableWebContentsImpl::RemoveFileSystem(const std::string& file_system_path) {
}

void InspectableWebContentsImpl::InspectedContentsClosing() {
}

void InspectableWebContentsImpl::RenderViewCreated(content::RenderViewHost* render_view_host) {
  content::DevToolsClientHost::SetupDevToolsFrontendClient(web_contents()->GetRenderViewHost());
  content::DevToolsManager::GetInstance()->RegisterDevToolsClientHostFor(agent_host_, frontend_host_.get());
}

void InspectableWebContentsImpl::WebContentsDestroyed(content::WebContents*) {
  content::DevToolsManager::GetInstance()->ClientHostClosing(frontend_host_.get());
  Observe(nullptr);
  agent_host_ = nullptr;
  frontend_host_.reset();
}

void InspectableWebContentsImpl::HandleKeyboardEvent(content::WebContents* source, const content::NativeWebKeyboardEvent& event) {
  auto delegate = web_contents_->GetDelegate();
  if (delegate)
    delegate->HandleKeyboardEvent(source, event);
}

}
