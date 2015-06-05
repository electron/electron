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
#include "base/json/json_writer.h"
#include "base/metrics/histogram.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/pref_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"

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

const char kDevToolsActionTakenHistogram[] = "DevTools.ActionTaken";
const int kDevToolsActionTakenBoundary = 100;
const char kDevToolsPanelShownHistogram[] = "DevTools.PanelShown";
const int kDevToolsPanelShownBoundary = 20;

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

// ResponseWriter -------------------------------------------------------------

class ResponseWriter : public net::URLFetcherResponseWriter {
 public:
  ResponseWriter(base::WeakPtr<InspectableWebContentsImpl> bindings, int stream_id);
  ~ResponseWriter() override;

  // URLFetcherResponseWriter overrides:
  int Initialize(const net::CompletionCallback& callback) override;
  int Write(net::IOBuffer* buffer,
            int num_bytes,
            const net::CompletionCallback& callback) override;
  int Finish(const net::CompletionCallback& callback) override;

 private:
  base::WeakPtr<InspectableWebContentsImpl> bindings_;
  int stream_id_;

  DISALLOW_COPY_AND_ASSIGN(ResponseWriter);
};

ResponseWriter::ResponseWriter(base::WeakPtr<InspectableWebContentsImpl> bindings,
                               int stream_id)
    : bindings_(bindings),
      stream_id_(stream_id) {
}

ResponseWriter::~ResponseWriter() {
}

int ResponseWriter::Initialize(const net::CompletionCallback& callback) {
  return net::OK;
}

int ResponseWriter::Write(net::IOBuffer* buffer,
                          int num_bytes,
                          const net::CompletionCallback& callback) {
  base::FundamentalValue* id = new base::FundamentalValue(stream_id_);
  base::StringValue* chunk =
      new base::StringValue(std::string(buffer->data(), num_bytes));

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&InspectableWebContentsImpl::CallClientFunction,
                 bindings_, "DevToolsAPI.streamWrite",
                 base::Owned(id), base::Owned(chunk), nullptr));
  return num_bytes;
}

int ResponseWriter::Finish(const net::CompletionCallback& callback) {
  return net::OK;
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
      delegate_(nullptr),
      weak_factory_(this) {
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
        DevToolsEmbedderMessageDispatcher::CreateForDevToolsFrontend(this));

    content::WebContents::CreateParams create_params(web_contents_->GetBrowserContext());
    devtools_web_contents_.reset(content::WebContents::Create(create_params));

    Observe(devtools_web_contents_.get());
    devtools_web_contents_->SetDelegate(this);

    agent_host_ = content::DevToolsAgentHost::GetOrCreateFor(web_contents_.get());
    frontend_host_.reset(content::DevToolsFrontendHost::Create(
        web_contents_->GetMainFrame(), this));
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

void InspectableWebContentsImpl::AttachTo(const scoped_refptr<content::DevToolsAgentHost>& host) {
  if (agent_host_.get())
    Detach();
  agent_host_ = host;
  agent_host_->AttachClient(this);
}

void InspectableWebContentsImpl::Detach() {
  agent_host_->DetachClient();
  agent_host_ = nullptr;
}

void InspectableWebContentsImpl::CallClientFunction(const std::string& function_name,
                                                    const base::Value* arg1,
                                                    const base::Value* arg2,
                                                    const base::Value* arg3) {
  std::string javascript = function_name + "(";
  if (arg1) {
    std::string json;
    base::JSONWriter::Write(arg1, &json);
    javascript.append(json);
    if (arg2) {
      base::JSONWriter::Write(arg2, &json);
      javascript.append(", ").append(json);
      if (arg3) {
        base::JSONWriter::Write(arg3, &json);
        javascript.append(", ").append(json);
      }
    }
  }
  javascript.append(");");
  devtools_web_contents_->GetMainFrame()->ExecuteJavaScript(
      base::UTF8ToUTF16(javascript));
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

void InspectableWebContentsImpl::LoadCompleted() {
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

void InspectableWebContentsImpl::InspectedURLChanged(const std::string& url) {
}

void InspectableWebContentsImpl::LoadNetworkResource(
    const DispatchCallback& callback,
    const std::string& url,
    const std::string& headers,
    int stream_id) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    base::DictionaryValue response;
    response.SetInteger("statusCode", 404);
    callback.Run(&response);
    return;
  }

  auto browser_context = static_cast<BrowserContext*>(devtools_web_contents_->GetBrowserContext());

  net::URLFetcher* fetcher =
      net::URLFetcher::Create(gurl, net::URLFetcher::GET, this);
  pending_requests_[fetcher] = callback;
  fetcher->SetRequestContext(browser_context->url_request_context_getter());
  fetcher->SetExtraRequestHeaders(headers);
  fetcher->SaveResponseWithWriter(scoped_ptr<net::URLFetcherResponseWriter>(
      new ResponseWriter(weak_factory_.GetWeakPtr(), stream_id)));
  fetcher->Start();
}

void InspectableWebContentsImpl::SetIsDocked(const DispatchCallback& callback,
                                             bool docked) {
  view_->SetIsDocked(docked);
  if (!callback.is_null())
    callback.Run(nullptr);
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
        base::ASCIIToUTF16("DevToolsAPI.fileSystemsLoaded([])"));
}

void InspectableWebContentsImpl::AddFileSystem() {
  if (delegate_)
    delegate_->DevToolsAddFileSystem();
}

void InspectableWebContentsImpl::RemoveFileSystem(
    const std::string& file_system_path) {
  if (delegate_)
    delegate_->DevToolsRemoveFileSystem(file_system_path);
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

void InspectableWebContentsImpl::SetWhitelistedShortcuts(const std::string& message) {
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

void InspectableWebContentsImpl::SetDevicesUpdatesEnabled(bool enabled) {
}

void InspectableWebContentsImpl::SendMessageToBrowser(const std::string& message) {
  if (agent_host_.get())
    agent_host_->DispatchProtocolMessage(message);
}

void InspectableWebContentsImpl::RecordActionUMA(const std::string& name, int action) {
  if (name == kDevToolsActionTakenHistogram)
    UMA_HISTOGRAM_ENUMERATION(name, action, kDevToolsActionTakenBoundary);
  else if (name == kDevToolsPanelShownHistogram)
    UMA_HISTOGRAM_ENUMERATION(name, action, kDevToolsPanelShownBoundary);
}

void InspectableWebContentsImpl::SendJsonRequest(const DispatchCallback& callback,
                                         const std::string& browser_id,
                                         const std::string& url) {
  callback.Run(nullptr);
}

void InspectableWebContentsImpl::HandleMessageFromDevToolsFrontend(const std::string& message) {
  std::string method;
  base::ListValue empty_params;
  base::ListValue* params = &empty_params;

  base::DictionaryValue* dict = NULL;
  scoped_ptr<base::Value> parsed_message(base::JSONReader::Read(message));
  if (!parsed_message ||
      !parsed_message->GetAsDictionary(&dict) ||
      !dict->GetString(kFrontendHostMethod, &method) ||
      (dict->HasKey(kFrontendHostParams) &&
          !dict->GetList(kFrontendHostParams, &params))) {
    LOG(ERROR) << "Invalid message was sent to embedder: " << message;
    return;
  }
  int id = 0;
  dict->GetInteger(kFrontendHostId, &id);
  embedder_message_dispatcher_->Dispatch(
      base::Bind(&InspectableWebContentsImpl::SendMessageAck,
                 weak_factory_.GetWeakPtr(),
                 id),
      method,
      params);
}

void InspectableWebContentsImpl::HandleMessageFromDevToolsFrontendToBackend(
    const std::string& message) {
  if (agent_host_.get())
    agent_host_->DispatchProtocolMessage(message);
}

void InspectableWebContentsImpl::DispatchProtocolMessage(
    content::DevToolsAgentHost* agent_host, const std::string& message) {
  std::string code = "DevToolsAPI.dispatchMessage(" + message + ");";
  base::string16 javascript = base::UTF8ToUTF16(code);
  web_contents()->GetMainFrame()->ExecuteJavaScript(javascript);
}

void InspectableWebContentsImpl::AgentHostClosed(
    content::DevToolsAgentHost* agent_host, bool replaced) {
}

void InspectableWebContentsImpl::AboutToNavigateRenderFrame(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  if (new_host->GetParent())
    return;
  frontend_host_.reset(content::DevToolsFrontendHost::Create(new_host, this));
}

void InspectableWebContentsImpl::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                               const GURL& validated_url) {
  if (render_frame_host->GetParent())
    return;

  view_->ShowDevTools();

  // If the devtools can dock, "SetIsDocked" will be called by devtools itself.
  if (!can_dock_)
    SetIsDocked(DispatchCallback(), false);
}

void InspectableWebContentsImpl::WebContentsDestroyed() {
  agent_host_->DetachClient();
  Observe(nullptr);
  agent_host_ = nullptr;

  for (const auto& pair : pending_requests_)
    delete pair.first;
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
    int main_frame_route_id,
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

void InspectableWebContentsImpl::WebContentsFocused(
    content::WebContents* contents) {
  if (delegate_)
    delegate_->DevToolsFocused();
}

void InspectableWebContentsImpl::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(source);
  PendingRequestsMap::iterator it = pending_requests_.find(source);
  DCHECK(it != pending_requests_.end());

  base::DictionaryValue response;
  base::DictionaryValue* headers = new base::DictionaryValue();
  net::HttpResponseHeaders* rh = source->GetResponseHeaders();
  response.SetInteger("statusCode", rh ? rh->response_code() : 200);
  response.Set("headers", headers);

  void* iterator = NULL;
  std::string name;
  std::string value;
  while (rh && rh->EnumerateHeaderLines(&iterator, &name, &value))
    headers->SetString(name, value);

  it->second.Run(&response);
  pending_requests_.erase(it);
  delete source;
}

void InspectableWebContentsImpl::SendMessageAck(int request_id,
                                                const base::Value* arg) {
  base::FundamentalValue id_value(request_id);
  CallClientFunction("DevToolsAPI.embedderMessageAck",
                     &id_value, arg, nullptr);
}

}  // namespace brightray
