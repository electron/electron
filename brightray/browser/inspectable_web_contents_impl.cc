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
#include "browser/inspectable_web_contents_view_delegate.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/pref_service.h"
#include "base/prefs/scoped_user_pref_update.h"
#include "base/strings/pattern.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/user_agent.h"
#include "ipc/ipc_channel.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "ui/gfx/screen.h"

namespace brightray {

namespace {

const double kPresetZoomFactors[] = { 0.25, 0.333, 0.5, 0.666, 0.75, 0.9, 1.0,
                                      1.1, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 4.0,
                                      5.0 };

const char kChromeUIDevToolsURL[] =
    "chrome-devtools://devtools/bundled/inspector.html?"
    "remoteBase=%s&"
    "can_dock=%s&"
    "toolbarColor=rgba(223,223,223,1)&"
    "textColor=rgba(0,0,0,1)&"
    "experiments=true";
const char kChromeUIDevToolsRemoteFrontendBase[] =
    "https://chrome-devtools-frontend.appspot.com/";
const char kChromeUIDevToolsRemoteFrontendPath[] = "serve_file";

const char kDevToolsBoundsPref[] = "brightray.devtools.bounds";
const char kDevToolsZoomPref[] = "brightray.devtools.zoom";
const char kDevToolsPreferences[] = "brightray.devtools.preferences";

const char kFrontendHostId[] = "id";
const char kFrontendHostMethod[] = "method";
const char kFrontendHostParams[] = "params";
const char kTitleFormat[] = "Developer Tools - %s";

const char kDevToolsActionTakenHistogram[] = "DevTools.ActionTaken";
const int kDevToolsActionTakenBoundary = 100;
const char kDevToolsPanelShownHistogram[] = "DevTools.PanelShown";
const int kDevToolsPanelShownBoundary = 20;

const size_t kMaxMessageChunkSize = IPC::Channel::kMaximumMessageSize / 4;

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

bool IsPointInRect(const gfx::Point& point, const gfx::Rect& rect) {
  return point.x() > rect.x() && point.x() < (rect.width() + rect.x()) &&
         point.y() > rect.y() && point.y() < (rect.height() + rect.y());
}

void SetZoomLevelForWebContents(content::WebContents* web_contents,
                                double level) {
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

GURL GetRemoteBaseURL() {
  return GURL(base::StringPrintf(
      "%s%s/%s/",
      kChromeUIDevToolsRemoteFrontendBase,
      kChromeUIDevToolsRemoteFrontendPath,
      content::GetWebKitRevision().c_str()));
}

GURL GetDevToolsURL(
    bool can_dock,
    const std::string& dock_state) {
  auto url_string =
      base::StringPrintf(kChromeUIDevToolsURL,
                         GetRemoteBaseURL().spec().c_str(),
                         can_dock ? "true" : "");
  if (!dock_state.empty())
    url_string += "&settings={\"currentDockState\":\"\\\"" + dock_state + "\\\"\"}&";
  return GURL(url_string);
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
  registry->RegisterDoublePref(kDevToolsZoomPref, 0.);
  registry->RegisterDictionaryPref(kDevToolsPreferences);
}

InspectableWebContentsImpl::InspectableWebContentsImpl(
    content::WebContents* web_contents)
    : frontend_loaded_(false),
      can_dock_(true),
      delegate_(nullptr),
      web_contents_(web_contents),
      weak_factory_(this) {
  auto context = static_cast<BrowserContext*>(web_contents_->GetBrowserContext());
  pref_service_ = context->prefs();
  auto bounds_dict = pref_service_->GetDictionary(kDevToolsBoundsPref);
  if (bounds_dict) {
    DictionaryToRect(*bounds_dict, &devtools_bounds_);
    // Sometimes the devtools window is out of screen or has too small size.
    if (devtools_bounds_.height() < 100 || devtools_bounds_.width() < 100) {
      devtools_bounds_.set_height(600);
      devtools_bounds_.set_width(800);
    }
    gfx::Rect display = gfx::Screen::GetNativeScreen()
        ->GetDisplayNearestWindow(web_contents->GetNativeView()).bounds();
    if (!IsPointInRect(devtools_bounds_.origin(), display)) {
      devtools_bounds_.set_x(display.x() + (display.width() - devtools_bounds_.width()) / 2);
      devtools_bounds_.set_y(display.y() + (display.height() - devtools_bounds_.height()) / 2);
    }
  }

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

content::WebContents* InspectableWebContentsImpl::GetDevToolsWebContents() const {
  return devtools_web_contents_.get();
}

void InspectableWebContentsImpl::SetDelegate(InspectableWebContentsDelegate* delegate) {
  delegate_ = delegate;
}

InspectableWebContentsDelegate* InspectableWebContentsImpl::GetDelegate() const {
  return delegate_;
}

void InspectableWebContentsImpl::SetDockState(const std::string& state) {
  if (state == "detach") {
    can_dock_ = false;
  } else {
    can_dock_ = true;
    dock_state_ = state;
  }
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
    agent_host_->AttachClient(this);

    devtools_web_contents_->GetController().LoadURL(
        GetDevToolsURL(can_dock_, dock_state_),
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
  if (agent_host_.get())
    agent_host_->DetachClient();
  agent_host_ = nullptr;
}

void InspectableWebContentsImpl::CallClientFunction(const std::string& function_name,
                                                    const base::Value* arg1,
                                                    const base::Value* arg2,
                                                    const base::Value* arg3) {
  if (!devtools_web_contents_)
    return;

  std::string javascript = function_name + "(";
  if (arg1) {
    std::string json;
    base::JSONWriter::Write(*arg1, &json);
    javascript.append(json);
    if (arg2) {
      base::JSONWriter::Write(*arg2, &json);
      javascript.append(", ").append(json);
      if (arg3) {
        base::JSONWriter::Write(*arg3, &json);
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
  base::DictionaryValue bounds_dict;
  RectToDictionary(bounds, &bounds_dict);
  pref_service_->Set(kDevToolsBoundsPref, bounds_dict);
  devtools_bounds_ = bounds;
}

double InspectableWebContentsImpl::GetDevToolsZoomLevel() const {
  return pref_service_->GetDouble(kDevToolsZoomPref);
}

void InspectableWebContentsImpl::UpdateDevToolsZoomLevel(double level) {
  pref_service_->SetDouble(kDevToolsZoomPref, level);
}

void InspectableWebContentsImpl::ActivateWindow() {
  // Set the zoom level.
  SetZoomLevelForWebContents(GetDevToolsWebContents(),
                             GetDevToolsZoomLevel());
}

void InspectableWebContentsImpl::CloseWindow() {
  GetDevToolsWebContents()->DispatchBeforeUnload(false);
}

void InspectableWebContentsImpl::LoadCompleted() {
  frontend_loaded_ = true;
  view_->ShowDevTools();

  // If the devtools can dock, "SetIsDocked" will be called by devtools itself.
  if (!can_dock_)
    SetIsDocked(DispatchCallback(), false);

  if (view_->GetDelegate())
    view_->GetDelegate()->DevToolsOpened();
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
  view_->SetTitle(base::UTF8ToUTF16(base::StringPrintf(kTitleFormat,
                                                       url.c_str())));
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
      (net::URLFetcher::Create(gurl, net::URLFetcher::GET, this)).release();
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
  if (delegate_)
    delegate_->DevToolsRequestFileSystems();
}

void InspectableWebContentsImpl::AddFileSystem(
    const std::string& file_system_path) {
  if (delegate_)
    delegate_->DevToolsAddFileSystem(
        base::FilePath::FromUTF8Unsafe(file_system_path));
}

void InspectableWebContentsImpl::RemoveFileSystem(
    const std::string& file_system_path) {
  if (delegate_)
    delegate_->DevToolsRemoveFileSystem(
        base::FilePath::FromUTF8Unsafe(file_system_path));
}

void InspectableWebContentsImpl::UpgradeDraggedFileSystemPermissions(
    const std::string& file_system_url) {
}

void InspectableWebContentsImpl::IndexPath(
    int request_id, const std::string& file_system_path) {
  if (delegate_)
    delegate_->DevToolsIndexPath(request_id, file_system_path);
}

void InspectableWebContentsImpl::StopIndexing(int request_id) {
  if (delegate_)
    delegate_->DevToolsStopIndexing(request_id);
}

void InspectableWebContentsImpl::SearchInPath(
    int request_id,
    const std::string& file_system_path,
    const std::string& query) {
  if (delegate_)
    delegate_->DevToolsSearchInPath(request_id, file_system_path, query);
}

void InspectableWebContentsImpl::SetWhitelistedShortcuts(const std::string& message) {
}

void InspectableWebContentsImpl::ZoomIn() {
  double new_level = GetNextZoomLevel(GetDevToolsZoomLevel(), false);
  SetZoomLevelForWebContents(GetDevToolsWebContents(), new_level);
  UpdateDevToolsZoomLevel(new_level);
}

void InspectableWebContentsImpl::ZoomOut() {
  double new_level = GetNextZoomLevel(GetDevToolsZoomLevel(), true);
  SetZoomLevelForWebContents(GetDevToolsWebContents(), new_level);
  UpdateDevToolsZoomLevel(new_level);
}

void InspectableWebContentsImpl::ResetZoom() {
  SetZoomLevelForWebContents(GetDevToolsWebContents(), 0.);
  UpdateDevToolsZoomLevel(0.);
}

void InspectableWebContentsImpl::SetDevicesUpdatesEnabled(bool enabled) {
}

void InspectableWebContentsImpl::DispatchProtocolMessageFromDevToolsFrontend(
    const std::string& message) {
  // If the devtools wants to reload the page, hijack the message and handle it
  // to the delegate.
  if (base::MatchPattern(message, "{\"id\":*,"
                                  "\"method\":\"Page.reload\","
                                  "\"params\":*}")) {
    if (delegate_)
      delegate_->DevToolsReloadPage();
    return;
  }

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

void InspectableWebContentsImpl::GetPreferences(
    const DispatchCallback& callback) {
  const base::DictionaryValue* prefs = pref_service_->GetDictionary(
      kDevToolsPreferences);
  callback.Run(prefs);
}

void InspectableWebContentsImpl::SetPreference(const std::string& name,
                                               const std::string& value) {
  DictionaryPrefUpdate update(pref_service_, kDevToolsPreferences);
  update.Get()->SetStringWithoutPathExpansion(name, value);
}

void InspectableWebContentsImpl::RemovePreference(const std::string& name) {
  DictionaryPrefUpdate update(pref_service_, kDevToolsPreferences);
  update.Get()->RemoveWithoutPathExpansion(name, nullptr);
}

void InspectableWebContentsImpl::ClearPreferences() {
  DictionaryPrefUpdate update(pref_service_, kDevToolsPreferences);
  update.Get()->Clear();
}

void InspectableWebContentsImpl::HandleMessageFromDevToolsFrontend(const std::string& message) {
  std::string method;
  base::ListValue empty_params;
  base::ListValue* params = &empty_params;

  base::DictionaryValue* dict = nullptr;
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

void InspectableWebContentsImpl::DispatchProtocolMessage(
    content::DevToolsAgentHost* agent_host, const std::string& message) {
  if (!frontend_loaded_)
    return;

  if (message.length() < kMaxMessageChunkSize) {
    base::string16 javascript = base::UTF8ToUTF16(
        "DevToolsAPI.dispatchMessage(" + message + ");");
    devtools_web_contents_->GetMainFrame()->ExecuteJavaScript(javascript);
    return;
  }

  base::FundamentalValue total_size(static_cast<int>(message.length()));
  for (size_t pos = 0; pos < message.length(); pos += kMaxMessageChunkSize) {
    base::StringValue message_value(message.substr(pos, kMaxMessageChunkSize));
    CallClientFunction("DevToolsAPI.dispatchMessageChunk",
                       &message_value, pos ? nullptr : &total_size, nullptr);
  }
}

void InspectableWebContentsImpl::AgentHostClosed(
    content::DevToolsAgentHost* agent_host, bool replaced) {
}

void InspectableWebContentsImpl::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  if (new_host->GetParent())
    return;
  frontend_host_.reset(content::DevToolsFrontendHost::Create(
      new_host,
      base::Bind(&InspectableWebContentsImpl::HandleMessageFromDevToolsFrontend,
                 base::Unretained(this))));
}

void InspectableWebContentsImpl::WebContentsDestroyed() {
  frontend_loaded_ = false;
  Observe(nullptr);
  Detach();

  for (const auto& pair : pending_requests_)
    delete pair.first;

  if (view_ && view_->GetDelegate())
    view_->GetDelegate()->DevToolsClosed();
}

bool InspectableWebContentsImpl::AddMessageToConsole(
    content::WebContents* source,
    int32_t level,
    const base::string16& message,
    int32_t line_no,
    const base::string16& source_id) {
  logging::LogMessage("CONSOLE", line_no, level).stream() << "\"" <<
      message << "\", source: " << source_id << " (" << line_no << ")";
  return true;
}

bool InspectableWebContentsImpl::ShouldCreateWebContents(
    content::WebContents* web_contents,
    int32_t route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    WindowContainerType window_container_type,
    const std::string& frame_name,
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

void InspectableWebContentsImpl::OnWebContentsFocused() {
#if defined(TOOLKIT_VIEWS)
  if (view_->GetDelegate())
    view_->GetDelegate()->DevToolsFocused();
#endif
}

void InspectableWebContentsImpl::DidStartNavigationToPendingEntry(
    const GURL& url,
    content::NavigationController::ReloadType reload_type) {
  frontend_host_.reset(
      content::DevToolsFrontendHost::Create(
          web_contents()->GetMainFrame(),
          base::Bind(&InspectableWebContentsImpl::HandleMessageFromDevToolsFrontend,
                     base::Unretained(this))));
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

  void* iterator = nullptr;
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
