// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include <set>

#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/native_window.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/event_emitter_caller.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/guest_host.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/plugin_service.h"
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
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_context.h"

#include "atom/common/node_includes.h"

namespace {

struct PrintSettings {
  bool silent;
  bool print_background;
};

}  // namespace

namespace mate {

template<>
struct Converter<atom::api::SetSizeParams> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::api::SetSizeParams* out) {
    mate::Dictionary params;
    if (!ConvertFromV8(isolate, val, &params))
      return false;
    bool autosize;
    if (params.Get("enableAutoSize", &autosize))
      out->enable_auto_size.reset(new bool(true));
    gfx::Size size;
    if (params.Get("min", &size))
      out->min_size.reset(new gfx::Size(size));
    if (params.Get("max", &size))
      out->max_size.reset(new gfx::Size(size));
    if (params.Get("normal", &size))
      out->normal_size.reset(new gfx::Size(size));
    return true;
  }
};

template<>
struct Converter<PrintSettings> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     PrintSettings* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    dict.Get("silent", &(out->silent));
    dict.Get("printBackground", &(out->print_background));
    return true;
  }
};

template<>
struct Converter<WindowOpenDisposition> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   WindowOpenDisposition val) {
    std::string disposition = "other";
    switch (val) {
      case CURRENT_TAB: disposition = "default"; break;
      case NEW_FOREGROUND_TAB: disposition = "foreground-tab"; break;
      case NEW_BACKGROUND_TAB: disposition = "background-tab"; break;
      case NEW_POPUP: case NEW_WINDOW: disposition = "new-window"; break;
      default: break;
    }
    return mate::ConvertToV8(isolate, disposition);
  }
};

}  // namespace mate


namespace atom {

namespace api {

namespace {

const int kDefaultWidth = 300;
const int kDefaultHeight = 300;

v8::Persistent<v8::ObjectTemplate> template_;

// The wrapWebContents funtion which is implemented in JavaScript
using WrapWebContentsCallback = base::Callback<void(v8::Local<v8::Value>)>;
WrapWebContentsCallback g_wrap_web_contents;

content::ServiceWorkerContext* GetServiceWorkerContext(
    const content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  auto site_instance = web_contents->GetSiteInstance();
  if (!context || !site_instance)
    return nullptr;

  auto storage_partition =
      content::BrowserContext::GetStoragePartition(context, site_instance);
  if (!storage_partition)
    return nullptr;

  return storage_partition->GetServiceWorkerContext();
}

}  // namespace

WebContents::WebContents(brightray::InspectableWebContents* web_contents)
    : WebContents(web_contents->GetWebContents()) {
  inspectable_web_contents_ = web_contents;
}

WebContents::WebContents(content::WebContents* web_contents)
    : CommonWebContentsDelegate(false),
      content::WebContentsObserver(web_contents),
      guest_opaque_(true),
      guest_host_(nullptr),
      auto_size_enabled_(false),
      is_full_page_plugin_(false),
      inspectable_web_contents_(nullptr) {
}

WebContents::WebContents(const mate::Dictionary& options)
    : CommonWebContentsDelegate(true),
      guest_opaque_(true),
      guest_host_(nullptr),
      auto_size_enabled_(false),
      is_full_page_plugin_(false) {
  auto browser_context = AtomBrowserMainParts::Get()->browser_context();
  content::SiteInstance* site_instance = content::SiteInstance::CreateForURL(
      browser_context, GURL("chrome-guest://fake-host"));

  content::WebContents::CreateParams params(browser_context, site_instance);
  params.guest_delegate = this;
  auto web_contents = content::WebContents::Create(params);

  NativeWindow* owner_window = nullptr;
  WebContents* embedder = nullptr;
  if (options.Get("embedder", &embedder) && embedder)
    owner_window = NativeWindow::FromWebContents(embedder->web_contents());

  InitWithWebContents(web_contents, owner_window);
  inspectable_web_contents_ = managed_web_contents();

  Observe(GetWebContents());
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
  Emit("new-window", target_url, frame_name, NEW_FOREGROUND_TAB);
  return false;
}

void WebContents::CloseContents(content::WebContents* source) {
  Emit("close");
}

content::WebContents* WebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (params.disposition != CURRENT_TAB) {
    Emit("new-window", params.url, "", params.disposition);
    return nullptr;
  }

  // Give user a chance to cancel navigation.
  if (Emit("will-navigate", params.url))
    return nullptr;

  return CommonWebContentsDelegate::OpenURLFromTab(source, params);
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

void WebContents::EnterFullscreenModeForTab(content::WebContents* source,
                                            const GURL& origin) {
  CommonWebContentsDelegate::EnterFullscreenModeForTab(source, origin);
  Emit("enter-html-full-screen");
}

void WebContents::ExitFullscreenModeForTab(content::WebContents* source) {
  CommonWebContentsDelegate::ExitFullscreenModeForTab(source);
  Emit("leave-html-full-screen");
}

void WebContents::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  int process_id = render_view_host->GetProcess()->GetID();
  Emit("render-view-deleted", process_id);

  // process.emit('ATOM_BROWSER_RELEASE_RENDER_VIEW', processId);
  // Tell the rpc server that a render view has been deleted and we need to
  // release all objects owned by it.
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  node::Environment* env = node::Environment::GetCurrent(isolate());
  mate::EmitEvent(isolate(), env->process_object(),
                  "ATOM_BROWSER_RELEASE_RENDER_VIEW", process_id);
}

void WebContents::RenderProcessGone(base::TerminationStatus status) {
  Emit("crashed");
}

void WebContents::PluginCrashed(const base::FilePath& plugin_path,
                                base::ProcessId plugin_pid) {
  content::WebPluginInfo info;
  auto plugin_service = content::PluginService::GetInstance();
  plugin_service->GetPluginInfoByPath(plugin_path, &info);
  Emit("plugin-crashed", info.name, info.version);
}

void WebContents::OnGpuProcessCrashed(base::TerminationStatus exit_code) {
  if (exit_code == base::TERMINATION_STATUS_PROCESS_CRASHED)
    Emit("gpu-crashed");
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

void WebContents::DidStartLoading() {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading() {
  Emit("did-stop-loading");
}

void WebContents::DidGetResourceResponseStart(
    const content::ResourceRequestDetails& details) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  base::DictionaryValue response_headers;

  net::HttpResponseHeaders* headers = details.headers.get();
  if (!headers)
    return;
  void* iter = nullptr;
  std::string key;
  std::string value;
  while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
    key = base::StringToLowerASCII(key);
    value = base::StringToLowerASCII(value);
    if (response_headers.HasKey(key)) {
      base::ListValue* values = nullptr;
      if (response_headers.GetList(key, &values))
        values->AppendString(value);
    } else {
      scoped_ptr<base::ListValue> values(new base::ListValue());
      values->AppendString(value);
      response_headers.Set(key, values.Pass());
    }
  }

  Emit("did-get-response-details",
       details.socket_address.IsEmpty(),
       details.url,
       details.original_url,
       details.http_response_code,
       details.method,
       details.referrer,
       response_headers);
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
  auto render_view_host_view = web_contents()->GetRenderViewHost()->GetView();
  if (guest_opaque_)
    render_view_host_view->SetBackgroundColorToDefault();
  else
    render_view_host_view->SetBackgroundColor(SK_ColorTRANSPARENT);
}

void WebContents::WebContentsDestroyed() {
  // The RenderViewDeleted was not called when the WebContents is destroyed.
  RenderViewDeleted(web_contents()->GetRenderViewHost());
  Emit("destroyed");
}

void WebContents::NavigationEntryCommitted(
    const content::LoadCommittedDetails& details) {
  Emit("navigation-entry-commited", details.entry->GetURL(),
       details.is_in_page, details.did_replace_entry);
}

void WebContents::DidAttach(int guest_proxy_routing_id) {
  Emit("did-attach");
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

void WebContents::SetGuestHost(content::GuestHost* guest_host) {
  guest_host_ = guest_host;
}

void WebContents::WillAttach(content::WebContents* embedder_web_contents,
                             int element_instance_id,
                             bool is_full_page_plugin) {
  embedder_web_contents_ = embedder_web_contents;
  is_full_page_plugin_ = is_full_page_plugin;
}

void WebContents::Destroy() {
  if (is_guest() && managed_web_contents()) {
    // When force destroying the "destroyed" event is not emitted.
    WebContentsDestroyed();

    // Give the content module an opportunity to perform some cleanup.
    guest_host_->WillDestroy();
    guest_host_ = nullptr;

    Observe(nullptr);
    DestroyWebContents();
  }
}

bool WebContents::IsAlive() const {
  return web_contents() != NULL;
}

int WebContents::GetID() const {
  return web_contents()->GetRenderProcessHost()->GetID();
}

bool WebContents::Equal(const WebContents* web_contents) const {
  return GetID() == web_contents->GetID();
}

void WebContents::LoadURL(const GURL& url, const mate::Dictionary& options) {
  content::NavigationController::LoadURLParams params(url);

  GURL http_referrer;
  if (options.Get("httpReferrer", &http_referrer))
    params.referrer = content::Referrer(http_referrer.GetAsReferrer(),
                                        blink::WebReferrerPolicyDefault);

  std::string user_agent;
  if (options.Get("userAgent", &user_agent))
    SetUserAgent(user_agent);

  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  params.should_clear_history_list = true;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  web_contents()->GetController().LoadURLWithParams(params);
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

void WebContents::ReloadIgnoringCache() {
  web_contents()->GetController().ReloadIgnoringCache(false);
}

void WebContents::GoBack() {
  atom::AtomBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoBack();
}

void WebContents::GoForward() {
  atom::AtomBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoForward();
}

void WebContents::GoToOffset(int offset) {
  atom::AtomBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoToOffset(offset);
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

void WebContents::OpenDevTools(mate::Arguments* args) {
  if (!inspectable_web_contents())
    return;
  bool detach = false;
  if (is_guest()) {
    detach = true;
  } else if (args && args->Length() == 1) {
    mate::Dictionary options;
    args->GetNext(&options) && options.Get("detach", &detach);
  }
  inspectable_web_contents()->SetCanDock(!detach);
  inspectable_web_contents()->ShowDevTools();
}

void WebContents::CloseDevTools() {
  if (!inspectable_web_contents())
    return;
  inspectable_web_contents()->CloseDevTools();
}

bool WebContents::IsDevToolsOpened() {
  if (!inspectable_web_contents())
    return false;
  return inspectable_web_contents()->IsDevToolsViewShowing();
}

void WebContents::ToggleDevTools() {
  if (IsDevToolsOpened())
    CloseDevTools();
  else
    OpenDevTools(nullptr);
}

void WebContents::InspectElement(int x, int y) {
  if (!inspectable_web_contents())
    return;
  OpenDevTools(nullptr);
  scoped_refptr<content::DevToolsAgentHost> agent(
    content::DevToolsAgentHost::GetOrCreateFor(web_contents()));
  agent->InspectElement(x, y);
}

void WebContents::InspectServiceWorker() {
  if (!inspectable_web_contents())
    return;
  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::TYPE_SERVICE_WORKER) {
      OpenDevTools(nullptr);
      inspectable_web_contents()->AttachTo(agent_host);
      break;
    }
  }
}

v8::Local<v8::Value> WebContents::Session(v8::Isolate* isolate) {
  if (session_.IsEmpty()) {
    mate::Handle<api::Session> handle = Session::Create(
        isolate,
        static_cast<AtomBrowserContext*>(web_contents()->GetBrowserContext()));
    session_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, session_);
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

void WebContents::SetAudioMuted(bool muted) {
  web_contents()->SetAudioMuted(muted);
}

bool WebContents::IsAudioMuted() {
  return web_contents()->IsAudioMuted();
}

void WebContents::Print(mate::Arguments* args) {
  PrintSettings settings = { false, false };
  if (args->Length() == 1 && !args->GetNext(&settings)) {
    args->ThrowError();
    return;
  }

  printing::PrintViewManagerBasic::FromWebContents(web_contents())->
      PrintNow(settings.silent, settings.print_background);
}

void WebContents::PrintToPDF(const base::DictionaryValue& setting,
                             const PrintToPDFCallback& callback) {
  printing::PrintPreviewMessageHandler::FromWebContents(web_contents())->
      PrintToPDF(setting, callback);
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

void WebContents::PasteAndMatchStyle() {
  web_contents()->PasteAndMatchStyle();
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

void WebContents::SetSize(const SetSizeParams& params) {
  bool enable_auto_size =
      params.enable_auto_size ? *params.enable_auto_size : auto_size_enabled_;
  gfx::Size min_size = params.min_size ? *params.min_size : min_auto_size_;
  gfx::Size max_size = params.max_size ? *params.max_size : max_auto_size_;

  if (params.normal_size)
    normal_size_ = *params.normal_size;

  min_auto_size_ = min_size;
  min_auto_size_.SetToMin(max_size);
  max_auto_size_ = max_size;
  max_auto_size_.SetToMax(min_size);

  enable_auto_size &= !min_auto_size_.IsEmpty() && !max_auto_size_.IsEmpty();

  content::RenderViewHost* rvh = web_contents()->GetRenderViewHost();
  if (enable_auto_size) {
    // Autosize is being enabled.
    rvh->EnableAutoResize(min_auto_size_, max_auto_size_);
    normal_size_.SetSize(0, 0);
  } else {
    // Autosize is being disabled.
    // Use default width/height if missing from partially defined normal size.
    if (normal_size_.width() && !normal_size_.height())
      normal_size_.set_height(GetDefaultSize().height());
    if (!normal_size_.width() && normal_size_.height())
      normal_size_.set_width(GetDefaultSize().width());

    gfx::Size new_size;
    if (!normal_size_.IsEmpty()) {
      new_size = normal_size_;
    } else if (!guest_size_.IsEmpty()) {
      new_size = guest_size_;
    } else {
      new_size = GetDefaultSize();
    }

    if (auto_size_enabled_) {
      // Autosize was previously enabled.
      rvh->DisableAutoResize(new_size);
      GuestSizeChangedDueToAutoSize(guest_size_, new_size);
    } else {
      // Autosize was already disabled.
      guest_host_->SizeContents(new_size);
    }

    guest_size_ = new_size;
  }

  auto_size_enabled_ = enable_auto_size;
}

void WebContents::SetAllowTransparency(bool allow) {
  if (guest_opaque_ != allow)
    return;

  auto render_view_host = web_contents()->GetRenderViewHost();
  guest_opaque_ = !allow;
  if (!render_view_host->GetView())
    return;

  if (guest_opaque_) {
    render_view_host->GetView()->SetBackgroundColorToDefault();
  } else {
    render_view_host->GetView()->SetBackgroundColor(SK_ColorTRANSPARENT);
  }
}

bool WebContents::IsGuest() const {
  return is_guest();
}

mate::ObjectTemplateBuilder WebContents::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  if (template_.IsEmpty())
    template_.Reset(isolate, mate::ObjectTemplateBuilder(isolate)
        .SetMethod("destroy", &WebContents::Destroy)
        .SetMethod("isAlive", &WebContents::IsAlive)
        .SetMethod("getId", &WebContents::GetID)
        .SetMethod("equal", &WebContents::Equal)
        .SetMethod("_loadUrl", &WebContents::LoadURL)
        .SetMethod("getTitle", &WebContents::GetTitle)
        .SetMethod("isLoading", &WebContents::IsLoading)
        .SetMethod("isWaitingForResponse", &WebContents::IsWaitingForResponse)
        .SetMethod("_stop", &WebContents::Stop)
        .SetMethod("_reloadIgnoringCache", &WebContents::ReloadIgnoringCache)
        .SetMethod("_goBack", &WebContents::GoBack)
        .SetMethod("_goForward", &WebContents::GoForward)
        .SetMethod("_goToOffset", &WebContents::GoToOffset)
        .SetMethod("isCrashed", &WebContents::IsCrashed)
        .SetMethod("setUserAgent", &WebContents::SetUserAgent)
        .SetMethod("insertCSS", &WebContents::InsertCSS)
        .SetMethod("_executeJavaScript", &WebContents::ExecuteJavaScript)
        .SetMethod("openDevTools", &WebContents::OpenDevTools)
        .SetMethod("closeDevTools", &WebContents::CloseDevTools)
        .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
        .SetMethod("toggleDevTools", &WebContents::ToggleDevTools)
        .SetMethod("inspectElement", &WebContents::InspectElement)
        .SetMethod("setAudioMuted", &WebContents::SetAudioMuted)
        .SetMethod("isAudioMuted", &WebContents::IsAudioMuted)
        .SetMethod("undo", &WebContents::Undo)
        .SetMethod("redo", &WebContents::Redo)
        .SetMethod("cut", &WebContents::Cut)
        .SetMethod("copy", &WebContents::Copy)
        .SetMethod("paste", &WebContents::Paste)
        .SetMethod("pasteAndMatchStyle", &WebContents::PasteAndMatchStyle)
        .SetMethod("delete", &WebContents::Delete)
        .SetMethod("selectAll", &WebContents::SelectAll)
        .SetMethod("unselect", &WebContents::Unselect)
        .SetMethod("replace", &WebContents::Replace)
        .SetMethod("replaceMisspelling", &WebContents::ReplaceMisspelling)
        .SetMethod("_send", &WebContents::SendIPCMessage)
        .SetMethod("setSize", &WebContents::SetSize)
        .SetMethod("setAllowTransparency", &WebContents::SetAllowTransparency)
        .SetMethod("isGuest", &WebContents::IsGuest)
        .SetMethod("hasServiceWorker", &WebContents::HasServiceWorker)
        .SetMethod("unregisterServiceWorker",
                   &WebContents::UnregisterServiceWorker)
        .SetMethod("inspectServiceWorker", &WebContents::InspectServiceWorker)
        .SetMethod("print", &WebContents::Print)
        .SetMethod("_printToPDF", &WebContents::PrintToPDF)
        .SetProperty("session", &WebContents::Session)
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

gfx::Size WebContents::GetDefaultSize() const {
  if (is_full_page_plugin_) {
    // Full page plugins default to the size of the owner's viewport.
    return embedder_web_contents_->GetRenderWidgetHostView()
                                 ->GetVisibleViewportSize();
  } else {
    return gfx::Size(kDefaultWidth, kDefaultHeight);
  }
}

// static
mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate, brightray::InspectableWebContents* web_contents) {
  auto handle = mate::CreateHandle(isolate, new WebContents(web_contents));
  g_wrap_web_contents.Run(handle.ToV8());
  return handle;
}

// static
mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate, content::WebContents* web_contents) {
  auto handle = mate::CreateHandle(isolate, new WebContents(web_contents));
  g_wrap_web_contents.Run(handle.ToV8());
  return handle;
}

// static
mate::Handle<WebContents> WebContents::Create(
    v8::Isolate* isolate, const mate::Dictionary& options) {
  auto handle =  mate::CreateHandle(isolate, new WebContents(options));
  g_wrap_web_contents.Run(handle.ToV8());
  return handle;
}

void SetWrapWebContents(const WrapWebContentsCallback& callback) {
  g_wrap_web_contents = callback;
}

void ClearWrapWebContents() {
  g_wrap_web_contents.Reset();
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("create", &atom::api::WebContents::Create);
  dict.SetMethod("_setWrapWebContents", &atom::api::SetWrapWebContents);
  dict.SetMethod("_clearWrapWebContents", &atom::api::ClearWrapWebContents);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_web_contents, Initialize)
