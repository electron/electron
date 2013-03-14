// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "devtools_frontend.h"

#include "browser/browser_client.h"
#include "browser/browser_main_parts.h"

#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_client_host.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/devtools_manager.h"
#include "content/public/browser/web_contents.h"

namespace brightray {

content::WebContents* DevToolsFrontend::Show(content::WebContents* inspected_contents) {
  // frontend will delete itself when the WebContents closes.
  auto frontend = new DevToolsFrontend(inspected_contents);

  return frontend->web_contents();
}

DevToolsFrontend::DevToolsFrontend(content::WebContents* inspected_contents)
  : WebContentsObserver(content::WebContents::Create(content::WebContents::CreateParams(inspected_contents->GetBrowserContext()))),
    inspected_web_contents_(inspected_contents),
    agent_host_(content::DevToolsAgentHost::GetFor(inspected_contents->GetRenderViewHost())),
    frontend_host_(content::DevToolsClientHost::CreateDevToolsFrontendHost(web_contents(), this)) {
  web_contents()->SetDelegate(this);
  auto client = static_cast<BrowserClient*>(content::GetContentClient()->browser());
  auto handler = client->browser_main_parts()->devtools_http_handler();
  auto url = handler->GetFrontendURL(nullptr);
  web_contents()->GetController().LoadURL(url, content::Referrer(), content::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());
}

DevToolsFrontend::~DevToolsFrontend() {
}

void DevToolsFrontend::ActivateWindow() {
}

void DevToolsFrontend::ChangeAttachedWindowHeight(unsigned height) {
}

void DevToolsFrontend::CloseWindow() {
}

void DevToolsFrontend::MoveWindow(int x, int y) {
}

void DevToolsFrontend::SetDockSide(const std::string& side) {
}

void DevToolsFrontend::OpenInNewTab(const std::string& url) {
}

void DevToolsFrontend::SaveToFile(const std::string& url, const std::string& content, bool save_as) {
}

void DevToolsFrontend::AppendToFile(const std::string& url, const std::string& content) {
}

void DevToolsFrontend::RequestFileSystems() {
}

void DevToolsFrontend::AddFileSystem() {
}

void DevToolsFrontend::RemoveFileSystem(const std::string& file_system_path) {
}

void DevToolsFrontend::InspectedContentsClosing() {
}

void DevToolsFrontend::RenderViewCreated(content::RenderViewHost* render_view_host) {
  content::DevToolsClientHost::SetupDevToolsFrontendClient(web_contents()->GetRenderViewHost());
  content::DevToolsManager::GetInstance()->RegisterDevToolsClientHostFor(agent_host_, frontend_host_.get());
}

void DevToolsFrontend::WebContentsDestroyed(content::WebContents*) {
  content::DevToolsManager::GetInstance()->ClientHostClosing(frontend_host_.get());
  delete this;
}

void DevToolsFrontend::HandleKeyboardEvent(content::WebContents* source, const content::NativeWebKeyboardEvent& event) {
  auto delegate = inspected_web_contents_->GetDelegate();
  if (delegate)
    delegate->HandleKeyboardEvent(source, event);
}

}
