// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/inspectable_web_contents_impl.h"

#include "browser/browser_client.h"
#include "browser/browser_context.h"
#include "browser/browser_main_parts.h"
#include "browser/inspectable_web_contents_delegate.h"
#include "browser/inspectable_web_contents_view.h"

#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/pref_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_client_host.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/devtools_manager.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/browser/render_view_host.h"

namespace brightray {

namespace {

const char kChromeUIDevToolsURL[] = "chrome-devtools://devtools/devtools.html";
const char kDockSidePref[] = "brightray.devtools.dockside";

}

// Implemented separately on each platform.
InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents_impl);

void InspectableWebContentsImpl::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kDockSidePref, "bottom");
}

InspectableWebContentsImpl::InspectableWebContentsImpl(
    content::WebContents* web_contents)
    : web_contents_(web_contents),
      delegate_(nullptr) {
  auto context = static_cast<BrowserContext*>(
      web_contents_->GetBrowserContext());
  dock_side_ = context->prefs()->GetString(kDockSidePref);

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
    embedder_message_dispatcher_.reset(
        new DevToolsEmbedderMessageDispatcher(this));

    auto create_params = content::WebContents::CreateParams(
        web_contents_->GetBrowserContext());
    devtools_web_contents_.reset(content::WebContents::Create(create_params));

#if defined(OS_MACOSX)
    // Work around http://crbug.com/279472.
    devtools_web_contents_->GetView()->SetAllowOverlappingViews(true);
#endif

    Observe(devtools_web_contents_.get());
    devtools_web_contents_->SetDelegate(this);

    agent_host_ = content::DevToolsAgentHost::GetOrCreateFor(
        web_contents_->GetRenderViewHost());
    frontend_host_.reset(
        content::DevToolsClientHost::CreateDevToolsFrontendHost(
            devtools_web_contents_.get(), this));
    content::DevToolsManager::GetInstance()->RegisterDevToolsClientHostFor(
        agent_host_, frontend_host_.get());

    GURL devtools_url(kChromeUIDevToolsURL);
    devtools_web_contents_->GetController().LoadURL(
        devtools_url,
        content::Referrer(),
        content::PAGE_TRANSITION_AUTO_TOPLEVEL,
        std::string());
  }

  if (delegate_ && delegate_->DevToolsShow(&dock_side_))
    return;

  view_->SetDockSide(dock_side_);
  view_->ShowDevTools();
}

void InspectableWebContentsImpl::CloseDevTools() {
  if (IsDevToolsViewShowing()) {
    view_->CloseDevTools();
    devtools_web_contents_.reset();
    web_contents_->GetView()->Focus();
  }
}

bool InspectableWebContentsImpl::IsDevToolsViewShowing() {
  return devtools_web_contents_ && view_->IsDevToolsViewShowing();
}

void InspectableWebContentsImpl::UpdateFrontendDockSide() {
  auto javascript = base::StringPrintf(
      "InspectorFrontendAPI.setDockSide(\"%s\")", dock_side_.c_str());
  devtools_web_contents_->GetRenderViewHost()->ExecuteJavascriptInWebFrame(
      base::string16(), base::ASCIIToUTF16(javascript));
}

void InspectableWebContentsImpl::ActivateWindow() {
}

void InspectableWebContentsImpl::CloseWindow() {
  CloseDevTools();
}

void InspectableWebContentsImpl::MoveWindow(int x, int y) {
}

void InspectableWebContentsImpl::SetDockSide(const std::string& side) {
  bool succeed = true;
  if (delegate_ && delegate_->DevToolsSetDockSide(side, &succeed)) {
    if (!succeed)  // delegate failed to set dock side.
      return;
  } else if (!view_->SetDockSide(side)) {
    return;
  }

  dock_side_ = side;

  auto context = static_cast<BrowserContext*>(
      web_contents_->GetBrowserContext());
  context->prefs()->SetString(kDockSidePref, side);

  UpdateFrontendDockSide();
}

void InspectableWebContentsImpl::OpenInNewTab(const std::string& url) {
}

void InspectableWebContentsImpl::SaveToFile(
    const std::string& url, const std::string& content, bool save_as) {
  if (delegate_)
    delegate_->DevToolsSaveToFile(url, content, save_as);
}

void InspectableWebContentsImpl::AppendToFile(
    const std::string& url, const std::string& content) {
  if (delegate_)
    delegate_->DevToolsAppendToFile(url, content);
}

void InspectableWebContentsImpl::RequestFileSystems() {
}

void InspectableWebContentsImpl::AddFileSystem() {
}

void InspectableWebContentsImpl::RemoveFileSystem(
    const std::string& file_system_path) {
}

void InspectableWebContentsImpl::IndexPath(
    int request_id, const std::string& file_system_path) {
}

void InspectableWebContentsImpl::StopIndexing(int request_id) {
}

void InspectableWebContentsImpl::SearchInPath(
    int request_id,
    const std::string& file_system_path,
    const std::string& query) {
}

void InspectableWebContentsImpl::DispatchOnEmbedder(
    const std::string& message) {
  embedder_message_dispatcher_->Dispatch(message);
}

void InspectableWebContentsImpl::InspectedContentsClosing() {
}

void InspectableWebContentsImpl::AboutToNavigateRenderView(
    content::RenderViewHost* render_view_host) {
  content::DevToolsClientHost::SetupDevToolsFrontendClient(
      web_contents()->GetRenderViewHost());
}

void InspectableWebContentsImpl::DidFinishLoad(int64 frame_id,
                                               const GURL& validated_url,
                                               bool is_main_frame,
                                               content::RenderViewHost*) {
  if (!is_main_frame)
    return;

  UpdateFrontendDockSide();
}

void InspectableWebContentsImpl::WebContentsDestroyed(content::WebContents*) {
  content::DevToolsManager::GetInstance()->ClientHostClosing(
      frontend_host_.get());
  Observe(nullptr);
  agent_host_ = nullptr;
  frontend_host_.reset();
}

void InspectableWebContentsImpl::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  auto delegate = web_contents_->GetDelegate();
  if (delegate)
    delegate->HandleKeyboardEvent(source, event);
}

}  // namespace brightray
