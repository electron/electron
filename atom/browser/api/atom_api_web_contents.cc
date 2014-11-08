// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

v8::Persistent<v8::ObjectTemplate> template_;

}  // namespace

WebContents::WebContents(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      guest_instance_id_(-1),
      guest_opaque_(true),
      auto_size_enabled_(false) {
}

WebContents::WebContents(const mate::Dictionary& options)
    : guest_instance_id_(-1),
      guest_opaque_(true),
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
  base::ListValue args;
  args.AppendInteger(level);
  args.AppendString(message);
  args.AppendInteger(line_no);
  args.AppendString(source_id);
  Emit("console-message", args);
  return true;
}

bool WebContents::ShouldCreateWebContents(
    content::WebContents* web_contents,
    int route_id,
    WindowContainerType window_container_type,
    const base::string16& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  base::ListValue args;
  args.AppendString(target_url.spec());
  args.AppendString(frame_name);
  args.AppendInteger(NEW_FOREGROUND_TAB);
  Emit("-new-window", args);
  return false;
}

void WebContents::CloseContents(content::WebContents* source) {
  Emit("close");
}

content::WebContents* WebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (params.disposition != CURRENT_TAB) {
    base::ListValue args;
    args.AppendString(params.url.spec());
    args.AppendString("");
    args.AppendInteger(params.disposition);
    Emit("-new-window", args);
    return NULL;
  }

  content::NavigationController::LoadURLParams load_url_params(params.url);
  load_url_params.referrer = params.referrer;
  load_url_params.transition_type = params.transition;
  load_url_params.extra_headers = params.extra_headers;
  load_url_params.should_replace_current_entry =
      params.should_replace_current_entry;
  load_url_params.is_renderer_initiated = params.is_renderer_initiated;
  load_url_params.transferred_global_request_id =
      params.transferred_global_request_id;

  web_contents()->GetController().LoadURLWithParams(load_url_params);
  return web_contents();
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
  base::ListValue args;
  args.AppendInteger(render_view_host->GetProcess()->GetID());
  args.AppendInteger(render_view_host->GetRoutingID());
  Emit("render-view-deleted", args);
}

void WebContents::RenderProcessGone(base::TerminationStatus status) {
  Emit("crashed");
}

void WebContents::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                const GURL& validated_url) {
  bool is_main_frame = !render_frame_host->GetParent();

  base::ListValue args;
  args.AppendBoolean(is_main_frame);
  Emit("did-frame-finish-load", args);

  if (is_main_frame)
    Emit("did-finish-load");
}

void WebContents::DidFailLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& validated_url,
                              int error_code,
                              const base::string16& error_description) {
  base::ListValue args;
  args.AppendInteger(error_code);
  args.AppendString(error_description);
  Emit("did-fail-load", args);
}

void WebContents::DidStartLoading(content::RenderViewHost* render_view_host) {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading(content::RenderViewHost* render_view_host) {
  Emit("did-stop-loading");
}

void WebContents::DidGetRedirectForResourceRequest(
    content::RenderViewHost* render_view_host,
    const content::ResourceRedirectDetails& details) {
  base::ListValue args;
  args.AppendString(details.url.spec());
  args.AppendString(details.new_url.spec());
  args.AppendBoolean(
      details.resource_type == content::RESOURCE_TYPE_MAIN_FRAME);
  Emit("did-get-redirect-request", args);
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
  web_contents()->GetRenderViewHost()->GetView()->
      SetBackgroundOpaque(guest_opaque_);

  content::RenderViewHost* rvh = web_contents()->GetRenderViewHost();
  if (auto_size_enabled_) {
    rvh->EnableAutoResize(min_auto_size_, max_auto_size_);
  } else {
    rvh->DisableAutoResize(element_size_);
  }
}

void WebContents::WebContentsDestroyed() {
  // The RenderViewDeleted was not called when the WebContents is destroyed.
  RenderViewDeleted(web_contents()->GetRenderViewHost());
  Emit("destroyed");
}

void WebContents::WillAttach(content::WebContents* embedder_web_contents,
                             const base::DictionaryValue& extra_params) {
  embedder_web_contents_ = embedder_web_contents;
  extra_params_.reset(extra_params.DeepCopy());
}

content::WebContents* WebContents::CreateNewGuestWindow(
    const content::WebContents::CreateParams& create_params) {
  NOTREACHED() << "Should not create new window from guest";
  return nullptr;
}

void WebContents::DidAttach() {
  base::ListValue args;
  args.Append(extra_params_.release());
  Emit("did-attach", args);
}

int WebContents::GetGuestInstanceID() const {
  return guest_instance_id_;
}

void WebContents::ElementSizeChanged(const gfx::Size& old_size,
                                     const gfx::Size& new_size) {
  element_size_ = new_size;
}

void WebContents::GuestSizeChanged(const gfx::Size& old_size,
                                   const gfx::Size& new_size) {
  if (!auto_size_enabled_)
    return;
  guest_size_ = new_size;
  GuestSizeChangedDueToAutoSize(old_size, new_size);
}

void WebContents::RequestPointerLockPermission(
    bool user_gesture,
    bool last_unlocked_by_target,
    const base::Callback<void(bool enabled)>& callback) {
  callback.Run(true);
}

void WebContents::RegisterDestructionCallback(
    const DestructionCallback& callback) {
  destruction_callback_ = callback;
}

void WebContents::Destroy() {
  if (storage_) {
    if (!destruction_callback_.is_null())
      destruction_callback_.Run();

    Observe(nullptr);
    storage_.reset();
  }
}

bool WebContents::IsAlive() const {
  return web_contents() != NULL;
}

void WebContents::LoadURL(const GURL& url, const mate::Dictionary& options) {
  content::NavigationController::LoadURLParams params(url);

  base::string16 http_referrer_;

  if (options.Get("httpreferrer", &http_referrer_))
    params.referrer = content::Referrer(GURL(http_referrer_).GetAsReferrer(), blink::WebReferrerPolicyDefault);

  params.transition_type = content::PAGE_TRANSITION_TYPED;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  web_contents()->GetController().LoadURLWithParams(params);
}

GURL WebContents::GetURL() const {
  return web_contents()->GetURL();
}

base::string16 WebContents::GetTitle() const {
  return web_contents()->GetTitle();
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

void WebContents::Reload(const mate::Dictionary& options) {
  // Navigating to a URL would always restart the renderer process, we want this
  // because normal reloading will break our node integration.
  // This is done by AtomBrowserClient::ShouldSwapProcessesForNavigation.
  LoadURL(GetURL(), options);
}

void WebContents::ReloadIgnoringCache(const mate::Dictionary& options) {
  Reload(options);
}

bool WebContents::CanGoBack() const {
  return web_contents()->GetController().CanGoBack();
}

bool WebContents::CanGoForward() const {
  return web_contents()->GetController().CanGoForward();
}

bool WebContents::CanGoToOffset(int offset) const {
  return web_contents()->GetController().CanGoToOffset(offset);
}

void WebContents::GoBack() {
  web_contents()->GetController().GoBack();
}

void WebContents::GoForward() {
  web_contents()->GetController().GoForward();
}

void WebContents::GoToIndex(int index) {
  web_contents()->GetController().GoToIndex(index);
}

void WebContents::GoToOffset(int offset) {
  web_contents()->GetController().GoToOffset(offset);
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

  web_contents()->GetRenderViewHost()->GetView()->SetBackgroundOpaque(!allow);
}

mate::ObjectTemplateBuilder WebContents::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  if (template_.IsEmpty())
    template_.Reset(isolate, mate::ObjectTemplateBuilder(isolate)
        .SetMethod("destroy", &WebContents::Destroy)
        .SetMethod("isAlive", &WebContents::IsAlive)
        .SetMethod("loadUrl", &WebContents::LoadURL)
        .SetMethod("getUrl", &WebContents::GetURL)
        .SetMethod("getTitle", &WebContents::GetTitle)
        .SetMethod("isLoading", &WebContents::IsLoading)
        .SetMethod("isWaitingForResponse", &WebContents::IsWaitingForResponse)
        .SetMethod("stop", &WebContents::Stop)
        .SetMethod("reload", &WebContents::Reload)
        .SetMethod("reloadIgnoringCache", &WebContents::ReloadIgnoringCache)
        .SetMethod("canGoBack", &WebContents::CanGoBack)
        .SetMethod("canGoForward", &WebContents::CanGoForward)
        .SetMethod("canGoToOffset", &WebContents::CanGoToOffset)
        .SetMethod("goBack", &WebContents::GoBack)
        .SetMethod("goForward", &WebContents::GoForward)
        .SetMethod("goToIndex", &WebContents::GoToIndex)
        .SetMethod("goToOffset", &WebContents::GoToOffset)
        .SetMethod("getRoutingId", &WebContents::GetRoutingID)
        .SetMethod("getProcessId", &WebContents::GetProcessID)
        .SetMethod("isCrashed", &WebContents::IsCrashed)
        .SetMethod("setUserAgent", &WebContents::SetUserAgent)
        .SetMethod("insertCSS", &WebContents::InsertCSS)
        .SetMethod("_executeJavaScript", &WebContents::ExecuteJavaScript)
        .SetMethod("_send", &WebContents::SendIPCMessage)
        .SetMethod("setAutoSize", &WebContents::SetAutoSize)
        .SetMethod("setAllowTransparency", &WebContents::SetAllowTransparency)
        .SetMethod("isGuest", &WebContents::is_guest)
        .SetMethod("openDevTools", &WebContents::OpenDevTools)
        .SetMethod("closeDevTools", &WebContents::CloseDevTools)
        .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
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
  Emit(base::UTF16ToUTF8(channel), args, web_contents(), message);
}

void WebContents::GuestSizeChangedDueToAutoSize(const gfx::Size& old_size,
                                                const gfx::Size& new_size) {
  base::ListValue args;
  args.AppendInteger(old_size.width());
  args.AppendInteger(old_size.height());
  args.AppendInteger(new_size.width());
  args.AppendInteger(new_size.height());
  Emit("size-changed", args);
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
