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

#include "base/json/json_reader.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/pref_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"

namespace brightray {

namespace {

const double kPresetZoomFactors[] = { 0.25, 0.333, 0.5, 0.666, 0.75, 0.9, 1.0,
                                      1.1, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 4.0,
                                      5.0 };

const char kChromeUIDevToolsURL[] = "chrome-devtools://devtools/devtools.html?"
                                    "can_dock=%s&"
                                    "toolbarColor=rgba(223,223,223,1)&"
                                    "textColor=rgba(0,0,0,1)&"
                                    "experiments=true";
const char kDevToolsBoundsPref[] = "brightray.devtools.bounds";

const char kFrontendHostId[] = "id";
const char kFrontendHostMethod[] = "method";
const char kFrontendHostParams[] = "params";

void RectToDictionary(const gfx::Rect& bounds, base::DictionaryValue* dict) {
  dict->SetInteger("x", bounds.x());
  dict->SetInteger("y", bounds.y());
  dict->SetInteger("width", bounds.width());
  dict->SetInteger("height", bounds.height());
}

void DictionaryToRect(const base::DictionaryValue& dict, gfx::Rect* bounds) {
  int x = 0, y = 0, width = 800, height = 600;
  dict.GetInteger("x", &x);
  dict.GetInteger("y", &y);
  dict.GetInteger("width", &width);
  dict.GetInteger("height", &height);
  *bounds = gfx::Rect(x, y, width, height);
}

bool ParseMessage(const std::string& message,
                  std::string* method,
                  base::ListValue* params,
                  int* id) {
  scoped_ptr<base::Value> parsed_message(base::JSONReader::Read(message));
  if (!parsed_message)
    return false;

  base::DictionaryValue* dict = NULL;
  if (!parsed_message->GetAsDictionary(&dict))
    return false;
  if (!dict->GetString(kFrontendHostMethod, method))
    return false;

  // "params" is optional.
  if (dict->HasKey(kFrontendHostParams)) {
    base::ListValue* internal_params;
    if (dict->GetList(kFrontendHostParams, &internal_params))
      params->Swap(internal_params);
    else
      return false;
  }

  *id = 0;
  dict->GetInteger(kFrontendHostId, id);
  return true;
}

double GetZoomLevelForWebContents(content::WebContents* web_contents) {
  return content::HostZoomMap::GetZoomLevel(web_contents);
}

void SetZoomLevelForWebContents(content::WebContents* web_contents, double level) {
  content::HostZoomMap::SetZoomLevel(web_contents, level);
}

double GetNextZoomLevel(double level, bool out) {
  double factor = content::ZoomLevelToZoomFactor(level);
  size_t size = arraysize(kPresetZoomFactors);
  for (size_t i = 0; i < size; ++i) {
    if (!content::ZoomValuesEqual(kPresetZoomFactors[i], factor))
      continue;
    if (out && i > 0)
      return content::ZoomFactorToZoomLevel(kPresetZoomFactors[i - 1]);
    if (!out && i != size - 1)
      return content::ZoomFactorToZoomLevel(kPresetZoomFactors[i + 1]);
  }
  return level;
}

}  // namespace

// Implemented separately on each platform.
InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents_impl);

void InspectableWebContentsImpl::RegisterPrefs(PrefRegistrySimple* registry) {
  auto bounds_dict = make_scoped_ptr(new base::DictionaryValue);
  RectToDictionary(gfx::Rect(0, 0, 800, 600), bounds_dict.get());
  registry->RegisterDictionaryPref(kDevToolsBoundsPref, bounds_dict.release());
}

InspectableWebContentsImpl::InspectableWebContentsImpl(
    content::WebContents* web_contents)
    : web_contents_(web_contents),
      can_dock_(true),
      delegate_(nullptr) {
  auto context = static_cast<BrowserContext*>(web_contents_->GetBrowserContext());
  auto bounds_dict = context->prefs()->GetDictionary(kDevToolsBoundsPref);
  if (bounds_dict)
    DictionaryToRect(*bounds_dict, &devtools_bounds_);

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

void InspectableWebContentsImpl::SetCanDock(bool can_dock) {
  can_dock_ = can_dock;
}

void InspectableWebContentsImpl::ShowDevTools() {
  // Show devtools only after it has done loading, this is to make sure the
  // SetIsDocked is called *BEFORE* ShowDevTools.
  if (!devtools_web_contents_) {
    embedder_message_dispatcher_.reset(
        new DevToolsEmbedderMessageDispatcher(this));

    content::WebContents::CreateParams create_params(web_contents_->GetBrowserContext());
    devtools_web_contents_.reset(content::WebContents::Create(create_params));

    Observe(devtools_web_contents_.get());
    devtools_web_contents_->SetDelegate(this);

    agent_host_ = content::DevToolsAgentHost::GetOrCreateFor(web_contents_.get());
    frontend_host_.reset(content::DevToolsFrontendHost::Create(
        web_contents_->GetRenderViewHost(), this));
    agent_host_->AttachClient(this);

    GURL devtools_url(base::StringPrintf(kChromeUIDevToolsURL, can_dock_ ? "true" : ""));
    devtools_web_contents_->GetController().LoadURL(
        devtools_url,
        content::Referrer(),
        ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
        std::string());
  } else {
    view_->ShowDevTools();
  }
}

void InspectableWebContentsImpl::CloseDevTools() {
  if (devtools_web_contents_) {
    view_->CloseDevTools();
    devtools_web_contents_.reset();
    web_contents_->Focus();
  }
}

bool InspectableWebContentsImpl::IsDevToolsViewShowing() {
  return devtools_web_contents_ && view_->IsDevToolsViewShowing();
}

gfx::Rect InspectableWebContentsImpl::GetDevToolsBounds() const {
  return devtools_bounds_;
}

void InspectableWebContentsImpl::SaveDevToolsBounds(const gfx::Rect& bounds) {
  auto context = static_cast<BrowserContext*>(web_contents_->GetBrowserContext());
  base::DictionaryValue bounds_dict;
  RectToDictionary(bounds, &bounds_dict);
  context->prefs()->Set(kDevToolsBoundsPref, bounds_dict);
  devtools_bounds_ = bounds;
}

void InspectableWebContentsImpl::ActivateWindow() {
}

void InspectableWebContentsImpl::CloseWindow() {
  devtools_web_contents()->DispatchBeforeUnload(false);
}

void InspectableWebContentsImpl::SetInspectedPageBounds(const gfx::Rect& rect) {
  DevToolsContentsResizingStrategy strategy(rect);
  if (contents_resizing_strategy_.Equals(strategy))
    return;

  contents_resizing_strategy_.CopyFrom(strategy);
  view_->SetContentsResizingStrategy(contents_resizing_strategy_);
}

void InspectableWebContentsImpl::InspectElementCompleted() {
}

void InspectableWebContentsImpl::MoveWindow(int x, int y) {
}

void InspectableWebContentsImpl::SetIsDocked(bool docked) {
  view_->SetIsDocked(docked);
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
    devtools_web_contents()->GetMainFrame()->ExecuteJavaScript(
        base::ASCIIToUTF16("InspectorFrontendAPI.fileSystemsLoaded([])"));
}

void InspectableWebContentsImpl::AddFileSystem() {
}

void InspectableWebContentsImpl::RemoveFileSystem(
    const std::string& file_system_path) {
}

void InspectableWebContentsImpl::UpgradeDraggedFileSystemPermissions(
    const std::string& file_system_url) {
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

void InspectableWebContentsImpl::ZoomIn() {
  double level = GetZoomLevelForWebContents(devtools_web_contents());
  SetZoomLevelForWebContents(devtools_web_contents(), GetNextZoomLevel(level, false));
}

void InspectableWebContentsImpl::ZoomOut() {
  double level = GetZoomLevelForWebContents(devtools_web_contents());
  SetZoomLevelForWebContents(devtools_web_contents(), GetNextZoomLevel(level, true));
}

void InspectableWebContentsImpl::ResetZoom() {
  SetZoomLevelForWebContents(devtools_web_contents(), 0.);
}

void InspectableWebContentsImpl::HandleMessageFromDevToolsFrontend(const std::string& message) {
  std::string method;
  base::ListValue params;
  int id;
  if (!ParseMessage(message, &method, &params, &id)) {
    LOG(ERROR) << "Invalid message was sent to embedder: " << message;
    return;
  }

  std::string error = embedder_message_dispatcher_->Dispatch(method, &params);
  if (id) {
    std::string ack = base::StringPrintf(
        "InspectorFrontendAPI.embedderMessageAck(%d, \"%s\");", id, error.c_str());
    devtools_web_contents()->GetMainFrame()->ExecuteJavaScript(base::UTF8ToUTF16(ack));
  }
}

void InspectableWebContentsImpl::HandleMessageFromDevToolsFrontendToBackend(
    const std::string& message) {
  agent_host_->DispatchProtocolMessage(message);
}

void InspectableWebContentsImpl::DispatchProtocolMessage(
    content::DevToolsAgentHost* agent_host, const std::string& message) {
  std::string code = "InspectorFrontendAPI.dispatchMessage(" + message + ");";
  base::string16 javascript = base::UTF8ToUTF16(code);
  web_contents()->GetMainFrame()->ExecuteJavaScript(javascript);
}

void InspectableWebContentsImpl::AgentHostClosed(
    content::DevToolsAgentHost* agent_host, bool replaced) {
}

void InspectableWebContentsImpl::AboutToNavigateRenderView(
    content::RenderViewHost* render_view_host) {
  frontend_host_.reset(content::DevToolsFrontendHost::Create(
      render_view_host, this));
}

void InspectableWebContentsImpl::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                               const GURL& validated_url) {
  if (render_frame_host->GetParent())
    return;

  view_->ShowDevTools();

  // If the devtools can dock, "SetIsDocked" will be called by devtools itself.
  if (!can_dock_)
    SetIsDocked(false);
}

void InspectableWebContentsImpl::WebContentsDestroyed() {
  agent_host_->DetachClient();
  Observe(nullptr);
  agent_host_ = nullptr;
}

bool InspectableWebContentsImpl::AddMessageToConsole(
    content::WebContents* source,
    int32 level,
    const base::string16& message,
    int32 line_no,
    const base::string16& source_id) {
  logging::LogMessage("CONSOLE", line_no, level).stream() << "\"" <<
      message << "\", source: " << source_id << " (" << line_no << ")";
  return true;
}

bool InspectableWebContentsImpl::ShouldCreateWebContents(
    content::WebContents* web_contents,
    int route_id,
    WindowContainerType window_container_type,
    const base::string16& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  return false;
}

void InspectableWebContentsImpl::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  auto delegate = web_contents_->GetDelegate();
  if (delegate)
    delegate->HandleKeyboardEvent(source, event);
}

void InspectableWebContentsImpl::CloseContents(content::WebContents* source) {
  CloseDevTools();
}

}  // namespace brightray
