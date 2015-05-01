// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include <set>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_javascript_dialog_manager.h"
#include "atom/browser/native_window.h"
#include "atom/browser/web_dialog_helper.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/media/media_stream_devices_controller.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/callback.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

v8::Persistent<v8::ObjectTemplate> template_;

// Get the window that has the |guest| embedded.
NativeWindow* GetWindowFromGuest(const content::WebContents* guest) {
  WebViewManager::WebViewInfo info;
  if (WebViewManager::GetInfoForProcess(guest->GetRenderProcessHost(), &info))
    return NativeWindow::FromRenderView(
        info.embedder->GetRenderProcessHost()->GetID(),
        info.embedder->GetRoutingID());
  else
    return nullptr;
}

content::ServiceWorkerContext* GetServiceWorkerContext(
    const content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  auto site_instance = web_contents->GetSiteInstance();
  if (!context || !site_instance)
    return nullptr;

  content::StoragePartition* storage_partition =
      content::BrowserContext::GetStoragePartition(
          context, site_instance);

  DCHECK(storage_partition);

  return storage_partition->GetServiceWorkerContext();
}

}  // namespace

WebContents::WebContents(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      guest_instance_id_(-1),
      element_instance_id_(-1),
      guest_opaque_(true),
      guest_sizer_(nullptr),
      auto_size_enabled_(false) {
}

WebContents::WebContents(const mate::Dictionary& options)
    : guest_instance_id_(-1),
      element_instance_id_(-1),
      guest_opaque_(true),
      guest_sizer_(nullptr),
      auto_size_enabled_(false) {
  options.Get("guestInstanceId", &guest_instance_id_);

  auto browser_context = AtomBrowserContext::Get();
  content::SiteInstance* site_instance = content::SiteInstance::CreateForURL(
      browser_context, GURL("chrome-guest://fake-host"));

  content::WebContents::CreateParams params(browser_context, site_instance);
  bool is_guest;
  if (options.Get("isGuest", &is_guest) && is_guest)
    params.guest_delegate = this;

  storage_.reset(brightray::InspectableWebContents::Create(params));
  Observe(storage_->GetWebContents());
  web_contents()->SetDelegate(this);
}

WebContents::~WebContents() {
  Destroy();
}

bool WebContents::AddMessageToConsole(content::WebContents* source,
                                      int32 level,
                                      const base::string16& message,
                                      int32 line_no,
                                      const base::string16& source_id) {
  Emit("console-message", level, message, line_no, source_id);
  return true;
}

bool WebContents::ShouldCreateWebContents(
    content::WebContents* web_contents,
    int route_id,
    int main_frame_route_id,
    WindowContainerType window_container_type,
    const base::string16& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  Emit("-new-window",
       target_url,
       frame_name,
       static_cast<int>(NEW_FOREGROUND_TAB));
  return false;
}

void WebContents::CloseContents(content::WebContents* source) {
  Emit("close");
}

content::WebContents* WebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (params.disposition != CURRENT_TAB) {
    Emit("-new-window", params.url, "", static_cast<int>(params.disposition));
    return nullptr;
  }

  // Give user a chance to cancel navigation.
  if (Emit("will-navigate", params.url))
    return nullptr;

  content::NavigationController::LoadURLParams load_url_params(params.url);
  load_url_params.referrer = params.referrer;
  load_url_params.transition_type = params.transition;
  load_url_params.extra_headers = params.extra_headers;
  load_url_params.should_replace_current_entry =
      params.should_replace_current_entry;
  load_url_params.is_renderer_initiated = params.is_renderer_initiated;
  load_url_params.transferred_global_request_id =
      params.transferred_global_request_id;
  load_url_params.should_clear_history_list = true;

  web_contents()->GetController().LoadURLWithParams(load_url_params);
  return web_contents();
}

content::JavaScriptDialogManager* WebContents::GetJavaScriptDialogManager(
    content::WebContents* source) {
  if (!dialog_manager_)
    dialog_manager_.reset(new AtomJavaScriptDialogManager);

  return dialog_manager_.get();
}

void WebContents::RunFileChooser(content::WebContents* guest,
                                 const content::FileChooserParams& params) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(GetWindowFromGuest(guest)));
  web_dialog_helper_->RunFileChooser(guest, params);
}

void WebContents::EnumerateDirectory(content::WebContents* guest,
                                     int request_id,
                                     const base::FilePath& path) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(GetWindowFromGuest(guest)));
  web_dialog_helper_->EnumerateDirectory(guest, request_id, path);
}

bool WebContents::CheckMediaAccessPermission(content::WebContents* web_contents,
                                             const GURL& security_origin,
                                             content::MediaStreamType type) {
  return true;
}

void WebContents::RequestMediaAccessPermission(
    content::WebContents*,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  brightray::MediaStreamDevicesController controller(request, callback);
  controller.TakeAction();
}

void WebContents::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (!attached())
    return;

  // Send the unhandled keyboard events back to the embedder to reprocess them.
  embedder_web_contents_->GetDelegate()->HandleKeyboardEvent(
      web_contents(), event);
}

void WebContents::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  Emit("render-view-deleted",
       render_view_host->GetProcess()->GetID(),
       render_view_host->GetRoutingID());
}

void WebContents::RenderProcessGone(base::TerminationStatus status) {
  Emit("crashed");
}

void WebContents::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host->GetParent())
    Emit("dom-ready");
}

void WebContents::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                const GURL& validated_url) {
  bool is_main_frame = !render_frame_host->GetParent();
  Emit("did-frame-finish-load", is_main_frame);

  if (is_main_frame)
    Emit("did-finish-load");
}

// this error occurs when host could not be found
void WebContents::DidFailProvisionalLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code,
    const base::string16& error_description) {
  Emit("did-fail-load", error_code, error_description);
}

void WebContents::DidFailLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& validated_url,
                              int error_code,
                              const base::string16& error_description) {
  Emit("did-fail-load", error_code, error_description);
}

void WebContents::DidStartLoading(content::RenderViewHost* render_view_host) {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading(content::RenderViewHost* render_view_host) {
  Emit("did-stop-loading");
}

void WebContents::DidGetResourceResponseStart(
    const content::ResourceRequestDetails& details) {
  Emit("did-get-response-details",
       details.socket_address.IsEmpty(),
       details.url,
       details.original_url,
       details.http_response_code,
       details.method,
       details.referrer);
}

void WebContents::DidGetRedirectForResourceRequest(
    content::RenderFrameHost* render_frame_host,
    const content::ResourceRedirectDetails& details) {
  Emit("did-get-redirect-request",
       details.url,
       details.new_url,
       (details.resource_type == content::RESOURCE_TYPE_MAIN_FRAME));
}

void WebContents::DidNavigateMainFrame(
    const content::LoadCommittedDetails& details,
    const content::FrameNavigateParams& params) {
  if (details.is_navigation_to_different_page())
    Emit("did-navigate-to-different-page");
}

void WebContents::TitleWasSet(content::NavigationEntry* entry,
                              bool explicit_set) {
  // Back/Forward navigation may have pruned entries.
  if (entry)
    Emit("page-title-set", entry->GetTitle(), explicit_set);
}

void WebContents::DidUpdateFaviconURL(
    const std::vector<content::FaviconURL>& urls) {
  std::set<GURL> unique_urls;
  for (auto iter = urls.begin(); iter != urls.end(); ++iter) {
    if (iter->icon_type != content::FaviconURL::FAVICON)
      continue;
    const GURL& url = iter->icon_url;
    if (url.is_valid())
      unique_urls.insert(url);
  }
  Emit("page-favicon-updated", unique_urls);
}

bool WebContents::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebContents, message)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AtomViewHostMsg_Message_Sync,
                                    OnRendererMessageSync)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void WebContents::RenderViewReady() {
  if (!is_guest())
    return;

  // We don't want to accidentally set the opacity of an interstitial page.
  // WebContents::GetRenderWidgetHostView will return the RWHV of an
  // interstitial page if one is showing at this time. We only want opacity
  // to apply to web pages.
  if (guest_opaque_) {
    web_contents()
        ->GetRenderViewHost()
        ->GetView()
        ->SetBackgroundColorToDefault();
  } else {
    web_contents()->GetRenderViewHost()->GetView()->SetBackgroundColor(
        SK_ColorTRANSPARENT);
  }
}

void WebContents::WebContentsDestroyed() {
  // The RenderViewDeleted was not called when the WebContents is destroyed.
  RenderViewDeleted(web_contents()->GetRenderViewHost());
  Emit("destroyed");
}

void WebContents::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  Emit("navigation-entry-commited", load_details.entry->GetURL());
}

void WebContents::DidAttach(int guest_proxy_routing_id) {
  Emit("did-attach");
}

void WebContents::ElementSizeChanged(const gfx::Size& size) {
  element_size_ = size;

  // Only resize if needed.
  if (!size.IsEmpty())
    guest_sizer_->SizeContents(size);
}

content::WebContents* WebContents::GetOwnerWebContents() const {
  return embedder_web_contents_;
}

void WebContents::GuestSizeChanged(const gfx::Size& new_size) {
  if (!auto_size_enabled_)
    return;
  GuestSizeChangedDueToAutoSize(guest_size_, new_size);
  guest_size_ = new_size;
}

void WebContents::RegisterDestructionCallback(
    const DestructionCallback& callback) {
  destruction_callback_ = callback;
}

void WebContents::SetGuestSizer(content::GuestSizer* guest_sizer) {
  guest_sizer_ = guest_sizer;
}

void WebContents::WillAttach(content::WebContents* embedder_web_contents,
                             int element_instance_id,
                             bool is_full_page_plugin) {
  embedder_web_contents_ = embedder_web_contents;
  element_instance_id_ = element_instance_id;
}

void WebContents::Destroy() {
  if (storage_) {
    if (!destruction_callback_.is_null())
      destruction_callback_.Run();

    // When force destroying the "destroyed" event is not emitted.
    WebContentsDestroyed();

    Observe(nullptr);
    storage_.reset();
  }
}

bool WebContents::IsAlive() const {
  return web_contents() != NULL;
}

void WebContents::LoadURL(const GURL& url, const mate::Dictionary& options) {
  content::NavigationController::LoadURLParams params(url);

  GURL http_referrer;
  if (options.Get("httpreferrer", &http_referrer))
    params.referrer = content::Referrer(http_referrer.GetAsReferrer(),
                                        blink::WebReferrerPolicyDefault);

  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  params.should_clear_history_list = true;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  web_contents()->GetController().LoadURLWithParams(params);
}

base::string16 WebContents::GetTitle() const {
  return web_contents()->GetTitle();
}

gfx::Image WebContents::GetFavicon() const {
  auto entry = web_contents()->GetController().GetLastCommittedEntry();
  if (!entry)
    return gfx::Image();
  return entry->GetFavicon().image;
}

bool WebContents::IsLoading() const {
  return web_contents()->IsLoading();
}

bool WebContents::IsWaitingForResponse() const {
  return web_contents()->IsWaitingForResponse();
}

void WebContents::Stop() {
  web_contents()->Stop();
}

void WebContents::ReloadIgnoringCache() {
  web_contents()->GetController().ReloadIgnoringCache(false);
}

int WebContents::GetRoutingID() const {
  return web_contents()->GetRoutingID();
}

int WebContents::GetProcessID() const {
  return web_contents()->GetRenderProcessHost()->GetID();
}

bool WebContents::IsCrashed() const {
  return web_contents()->IsCrashed();
}

void WebContents::SetUserAgent(const std::string& user_agent) {
  web_contents()->SetUserAgentOverride(user_agent);
}

void WebContents::InsertCSS(const std::string& css) {
  web_contents()->InsertCSS(css);
}

void WebContents::ExecuteJavaScript(const base::string16& code) {
  web_contents()->GetMainFrame()->ExecuteJavaScript(code);
}

void WebContents::OpenDevTools() {
  storage_->SetCanDock(false);
  storage_->ShowDevTools();
}

void WebContents::CloseDevTools() {
  storage_->CloseDevTools();
}

bool WebContents::IsDevToolsOpened() {
  return storage_->IsDevToolsViewShowing();
}

void WebContents::InspectElement(int x, int y) {
  OpenDevTools();
  scoped_refptr<content::DevToolsAgentHost> agent(
    content::DevToolsAgentHost::GetOrCreateFor(storage_->GetWebContents()));
  agent->InspectElement(x, y);
}

void WebContents::Undo() {
  web_contents()->Undo();
}

void WebContents::Redo() {
  web_contents()->Redo();
}

void WebContents::Cut() {
  web_contents()->Cut();
}

void WebContents::Copy() {
  web_contents()->Copy();
}

void WebContents::Paste() {
  web_contents()->Paste();
}

void WebContents::Delete() {
  web_contents()->Delete();
}

void WebContents::SelectAll() {
  web_contents()->SelectAll();
}

void WebContents::Unselect() {
  web_contents()->Unselect();
}

void WebContents::Replace(const base::string16& word) {
  web_contents()->Replace(word);
}

void WebContents::ReplaceMisspelling(const base::string16& word) {
  web_contents()->ReplaceMisspelling(word);
}

bool WebContents::SendIPCMessage(const base::string16& channel,
                                 const base::ListValue& args) {
  return Send(new AtomViewMsg_Message(routing_id(), channel, args));
}

void WebContents::SetAutoSize(bool enabled,
                              const gfx::Size& min_size,
                              const gfx::Size& max_size) {
  min_auto_size_ = min_size;
  min_auto_size_.SetToMin(max_size);
  max_auto_size_ = max_size;
  max_auto_size_.SetToMax(min_size);

  enabled &= !min_auto_size_.IsEmpty() && !max_auto_size_.IsEmpty();
  if (!enabled && !auto_size_enabled_)
    return;

  auto_size_enabled_ = enabled;

  if (!attached())
    return;

  content::RenderViewHost* rvh = web_contents()->GetRenderViewHost();
  if (auto_size_enabled_) {
    rvh->EnableAutoResize(min_auto_size_, max_auto_size_);
  } else {
    rvh->DisableAutoResize(element_size_);
    guest_size_ = element_size_;
    GuestSizeChangedDueToAutoSize(guest_size_, element_size_);
  }
}

void WebContents::SetAllowTransparency(bool allow) {
  if (guest_opaque_ != allow)
    return;

  guest_opaque_ = !allow;
  if (!web_contents()->GetRenderViewHost()->GetView())
    return;

  if (guest_opaque_) {
    web_contents()
        ->GetRenderViewHost()
        ->GetView()
        ->SetBackgroundColorToDefault();
  } else {
    web_contents()->GetRenderViewHost()->GetView()->SetBackgroundColor(
        SK_ColorTRANSPARENT);
  }
}

void WebContents::HasServiceWorker(
    const base::Callback<void(bool)>& callback) {
  auto context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->CheckHasServiceWorker(web_contents()->GetLastCommittedURL(),
                                 GURL::EmptyGURL(),
                                 callback);
}

void WebContents::UnregisterServiceWorker(
    const base::Callback<void(bool)>& callback) {
  auto context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->UnregisterServiceWorker(web_contents()->GetLastCommittedURL(),
                                   callback);
}

mate::ObjectTemplateBuilder WebContents::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  if (template_.IsEmpty())
    template_.Reset(isolate, mate::ObjectTemplateBuilder(isolate)
        .SetMethod("destroy", &WebContents::Destroy)
        .SetMethod("isAlive", &WebContents::IsAlive)
        .SetMethod("_loadUrl", &WebContents::LoadURL)
        .SetMethod("getTitle", &WebContents::GetTitle)
        .SetMethod("getFavicon", &WebContents::GetFavicon)
        .SetMethod("isLoading", &WebContents::IsLoading)
        .SetMethod("isWaitingForResponse", &WebContents::IsWaitingForResponse)
        .SetMethod("_stop", &WebContents::Stop)
        .SetMethod("_reloadIgnoringCache", &WebContents::ReloadIgnoringCache)
        .SetMethod("getRoutingId", &WebContents::GetRoutingID)
        .SetMethod("getProcessId", &WebContents::GetProcessID)
        .SetMethod("isCrashed", &WebContents::IsCrashed)
        .SetMethod("setUserAgent", &WebContents::SetUserAgent)
        .SetMethod("insertCSS", &WebContents::InsertCSS)
        .SetMethod("_executeJavaScript", &WebContents::ExecuteJavaScript)
        .SetMethod("openDevTools", &WebContents::OpenDevTools)
        .SetMethod("closeDevTools", &WebContents::CloseDevTools)
        .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
        .SetMethod("inspectElement", &WebContents::InspectElement)
        .SetMethod("undo", &WebContents::Undo)
        .SetMethod("redo", &WebContents::Redo)
        .SetMethod("cut", &WebContents::Cut)
        .SetMethod("copy", &WebContents::Copy)
        .SetMethod("paste", &WebContents::Paste)
        .SetMethod("delete", &WebContents::Delete)
        .SetMethod("selectAll", &WebContents::SelectAll)
        .SetMethod("unselect", &WebContents::Unselect)
        .SetMethod("replace", &WebContents::Replace)
        .SetMethod("replaceMisspelling", &WebContents::ReplaceMisspelling)
        .SetMethod("_send", &WebContents::SendIPCMessage)
        .SetMethod("setAutoSize", &WebContents::SetAutoSize)
        .SetMethod("setAllowTransparency", &WebContents::SetAllowTransparency)
        .SetMethod("isGuest", &WebContents::is_guest)
        .SetMethod("hasServiceWorker", &WebContents::HasServiceWorker)
        .SetMethod("unregisterServiceWorker",
                   &WebContents::UnregisterServiceWorker)
        .Build());

  return mate::ObjectTemplateBuilder(
      isolate, v8::Local<v8::ObjectTemplate>::New(isolate, template_));
}

void WebContents::OnRendererMessage(const base::string16& channel,
                                    const base::ListValue& args) {
  // webContents.emit(channel, new Event(), args...);
  Emit(base::UTF16ToUTF8(channel), args);
}

void WebContents::OnRendererMessageSync(const base::string16& channel,
                                        const base::ListValue& args,
                                        IPC::Message* message) {
  // webContents.emit(channel, new Event(sender, message), args...);
  EmitWithSender(base::UTF16ToUTF8(channel), web_contents(), message, args);
}

void WebContents::GuestSizeChangedDueToAutoSize(const gfx::Size& old_size,
                                                const gfx::Size& new_size) {
  Emit("size-changed",
       old_size.width(), old_size.height(),
       new_size.width(), new_size.height());
}

// static
mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate, content::WebContents* web_contents) {
  return mate::CreateHandle(isolate, new WebContents(web_contents));
}

// static
mate::Handle<WebContents> WebContents::Create(
    v8::Isolate* isolate, const mate::Dictionary& options) {
  return mate::CreateHandle(isolate, new WebContents(options));
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("create", &atom::api::WebContents::Create);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_web_contents, Initialize)
