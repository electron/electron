// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include <set>

#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/api/atom_api_window.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/native_window.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/web_view_guest_delegate.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/blink_converter.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_plugin_guest_manager.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/native_web_keyboard_event.h"
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
#include "content/public/common/context_menu_params.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "ui/base/l10n/l10n_util.h"

#include "atom/common/node_includes.h"

namespace {

struct PrintSettings {
  bool silent;
  bool print_background;
};

void SetUserAgentInIO(scoped_refptr<net::URLRequestContextGetter> getter,
                      std::string accept_lang,
                      std::string user_agent) {
  getter->GetURLRequestContext()->set_http_user_agent_settings(
      new net::StaticHttpUserAgentSettings(
          net::HttpUtil::GenerateAcceptLanguageHeader(accept_lang),
          user_agent));
}

bool NotifyZoomLevelChanged(
    double level, content::WebContents* guest_web_contents) {
  guest_web_contents->SendToAllFrames(
      new AtomViewMsg_SetZoomLevel(MSG_ROUTING_NONE, level));

  // Return false to iterate over all guests.
  return false;
}

}  // namespace

namespace mate {

template<>
struct Converter<atom::SetSizeParams> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::SetSizeParams* out) {
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

template<>
struct Converter<net::HttpResponseHeaders*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   net::HttpResponseHeaders* headers) {
    base::DictionaryValue response_headers;
    if (headers) {
      void* iter = nullptr;
      std::string key;
      std::string value;
      while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::StringToLowerASCII(key);
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
    }
    return ConvertToV8(isolate, response_headers);
  }
};

template<>
struct Converter<content::SavePageType> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::SavePageType* out) {
    std::string save_type;
    if (!ConvertFromV8(isolate, val, &save_type))
      return false;
    save_type = base::StringToLowerASCII(save_type);
    if (save_type == "htmlonly") {
      *out = content::SAVE_PAGE_TYPE_AS_ONLY_HTML;
    } else if (save_type == "htmlcomplete") {
      *out = content::SAVE_PAGE_TYPE_AS_COMPLETE_HTML;
    } else if (save_type == "mhtml") {
      *out = content::SAVE_PAGE_TYPE_AS_MHTML;
    } else {
      return false;
    }
    return true;
  }
};

}  // namespace mate


namespace atom {

namespace api {

namespace {

v8::Persistent<v8::ObjectTemplate> template_;

// The wrapWebContents function which is implemented in JavaScript
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

WebContents::WebContents(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      type_(REMOTE) {
  AttachAsUserData(web_contents);
  web_contents->SetUserAgentOverride(GetBrowserContext()->GetUserAgent());
}

WebContents::WebContents(v8::Isolate* isolate,
                         const mate::Dictionary& options) {
  // Whether it is a guest WebContents.
  bool is_guest = false;
  options.Get("isGuest", &is_guest);
  type_ = is_guest ? WEB_VIEW : BROWSER_WINDOW;

  // Obtain the session.
  std::string partition;
  mate::Handle<api::Session> session;
  if (options.Get("session", &session)) {
  } else if (options.Get("partition", &partition) && !partition.empty()) {
    bool in_memory = true;
    if (base::StartsWith(partition, "persist:", base::CompareCase::SENSITIVE)) {
      in_memory = false;
      partition = partition.substr(8);
    }
    session = Session::FromPartition(isolate, partition, in_memory);
  } else {
    // Use the default session if not specified.
    session = Session::FromPartition(isolate, "", false);
  }
  session_.Reset(isolate, session.ToV8());

  content::WebContents* web_contents;
  if (is_guest) {
    content::SiteInstance* site_instance = content::SiteInstance::CreateForURL(
        session->browser_context(), GURL("chrome-guest://fake-host"));
    content::WebContents::CreateParams params(
        session->browser_context(), site_instance);
    guest_delegate_.reset(new WebViewGuestDelegate);
    params.guest_delegate = guest_delegate_.get();
    web_contents = content::WebContents::Create(params);
  } else {
    content::WebContents::CreateParams params(session->browser_context());
    web_contents = content::WebContents::Create(params);
  }

  Observe(web_contents);
  AttachAsUserData(web_contents);
  InitWithWebContents(web_contents);

  managed_web_contents()->GetView()->SetDelegate(this);

  // Save the preferences in C++.
  base::DictionaryValue web_preferences;
  mate::ConvertFromV8(isolate, options.GetHandle(), &web_preferences);
  new WebContentsPreferences(web_contents, &web_preferences);

  web_contents->SetUserAgentOverride(GetBrowserContext()->GetUserAgent());

  if (is_guest) {
    guest_delegate_->Initialize(this);

    NativeWindow* owner_window = nullptr;
    WebContents* embedder = nullptr;
    if (options.Get("embedder", &embedder) && embedder) {
      // New WebContents's owner_window is the embedder's owner_window.
      auto relay = NativeWindowRelay::FromWebContents(embedder->web_contents());
      if (relay)
        owner_window = relay->window.get();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);
  }
}

WebContents::~WebContents() {
  Destroy();
}

bool WebContents::AddMessageToConsole(content::WebContents* source,
                                      int32 level,
                                      const base::string16& message,
                                      int32 line_no,
                                      const base::string16& source_id) {
  if (type_ == BROWSER_WINDOW) {
    return false;
  } else {
    Emit("console-message", level, message, line_no, source_id);
    return true;
  }
}

bool WebContents::ShouldCreateWebContents(
    content::WebContents* web_contents,
    int route_id,
    int main_frame_route_id,
    WindowContainerType window_container_type,
    const std::string& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  if (type_ == BROWSER_WINDOW)
    Emit("-new-window", target_url, frame_name, NEW_FOREGROUND_TAB);
  else
    Emit("new-window", target_url, frame_name, NEW_FOREGROUND_TAB);
  return false;
}

content::WebContents* WebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (params.disposition != CURRENT_TAB) {
    if (type_ == BROWSER_WINDOW)
      Emit("-new-window", params.url, "", params.disposition);
    else
      Emit("new-window", params.url, "", params.disposition);
    return nullptr;
  }

  // Give user a chance to cancel navigation.
  if (Emit("will-navigate", params.url))
    return nullptr;

  return CommonWebContentsDelegate::OpenURLFromTab(source, params);
}

void WebContents::BeforeUnloadFired(content::WebContents* tab,
                                    bool proceed,
                                    bool* proceed_to_fire_unload) {
  if (type_ == BROWSER_WINDOW)
    *proceed_to_fire_unload = proceed;
  else
    *proceed_to_fire_unload = true;
}

void WebContents::MoveContents(content::WebContents* source,
                               const gfx::Rect& pos) {
  Emit("move", pos);
}

void WebContents::CloseContents(content::WebContents* source) {
  Emit("close");
  if (type_ == BROWSER_WINDOW)
    owner_window()->CloseContents(source);
}

void WebContents::ActivateContents(content::WebContents* source) {
  Emit("activate");
}

bool WebContents::IsPopupOrPanel(const content::WebContents* source) const {
  return type_ == BROWSER_WINDOW;
}

void WebContents::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (event.windowsKeyCode == ui::VKEY_ESCAPE && is_html_fullscreen()) {
    // Escape exits tabbed fullscreen mode.
    ExitFullscreenModeForTab(source);
  } else if (type_ == BROWSER_WINDOW) {
    owner_window()->HandleKeyboardEvent(source, event);
  } else if (type_ == WEB_VIEW && guest_delegate_) {
    // Send the unhandled keyboard events back to the embedder.
    guest_delegate_->HandleKeyboardEvent(source, event);
  }
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

void WebContents::RendererUnresponsive(content::WebContents* source) {
  Emit("unresponsive");
  if (type_ == BROWSER_WINDOW)
    owner_window()->RendererUnresponsive(source);
}

void WebContents::RendererResponsive(content::WebContents* source) {
  Emit("responsive");
  if (type_ == BROWSER_WINDOW)
    owner_window()->RendererResponsive(source);
}

bool WebContents::HandleContextMenu(const content::ContextMenuParams& params) {
  if (!params.custom_context.is_pepper_menu)
    return false;

  Emit("pepper-context-menu", std::make_pair(params, web_contents()));
  web_contents()->NotifyContextMenuClosed(params.custom_context);
  return true;
}

void WebContents::BeforeUnloadFired(const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
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
    const base::string16& error_description,
    bool was_ignored_by_handler) {
  Emit("did-fail-load", error_code, error_description, validated_url);
}

void WebContents::DidFailLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& validated_url,
                              int error_code,
                              const base::string16& error_description,
                              bool was_ignored_by_handler) {
  Emit("did-fail-load", error_code, error_description, validated_url);
}

void WebContents::DidStartLoading() {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading() {
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
       details.referrer,
       details.headers.get());
}

void WebContents::DidGetRedirectForResourceRequest(
    content::RenderFrameHost* render_frame_host,
    const content::ResourceRedirectDetails& details) {
  Emit("did-get-redirect-request",
       details.url,
       details.new_url,
       (details.resource_type == content::RESOURCE_TYPE_MAIN_FRAME),
       details.http_response_code,
       details.method,
       details.referrer,
       details.headers.get());
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

void WebContents::DevToolsFocused() {
  Emit("devtools-focused");
}

void WebContents::DevToolsOpened() {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto handle = WebContents::CreateFrom(
      isolate(), managed_web_contents()->GetDevToolsWebContents());
  devtools_web_contents_.Reset(isolate(), handle.ToV8());

  // Inherit owner window in devtools.
  if (owner_window())
    handle->SetOwnerWindow(managed_web_contents()->GetDevToolsWebContents(),
                           owner_window());

  Emit("devtools-opened");
}

void WebContents::DevToolsClosed() {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  devtools_web_contents_.Reset();

  Emit("devtools-closed");
}

bool WebContents::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebContents, message)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AtomViewHostMsg_Message_Sync,
                                    OnRendererMessageSync)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_ZoomLevelChanged, OnZoomLevelChanged)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void WebContents::WebContentsDestroyed() {
  // The RenderViewDeleted was not called when the WebContents is destroyed.
  RenderViewDeleted(web_contents()->GetRenderViewHost());
  Emit("destroyed");
  RemoveFromWeakMap();
}

void WebContents::NavigationEntryCommitted(
    const content::LoadCommittedDetails& details) {
  Emit("navigation-entry-commited", details.entry->GetURL(),
       details.is_in_page, details.did_replace_entry);
}

void WebContents::Destroy() {
  session_.Reset();
  if (type_ == WEB_VIEW && managed_web_contents()) {
    // When force destroying the "destroyed" event is not emitted.
    WebContentsDestroyed();

    guest_delegate_->Destroy();

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

  std::string extra_headers;
  if (options.Get("extraHeaders", &extra_headers))
    params.extra_headers = extra_headers;

  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  params.should_clear_history_list = true;
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
  scoped_refptr<net::URLRequestContextGetter> getter =
      web_contents()->GetBrowserContext()->GetRequestContext();

  auto accept_lang = l10n_util::GetApplicationLocale("");
  getter->GetNetworkTaskRunner()->PostTask(FROM_HERE,
      base::Bind(&SetUserAgentInIO, getter, accept_lang, user_agent));
}

std::string WebContents::GetUserAgent() {
  return web_contents()->GetUserAgentOverride();
}

void WebContents::InsertCSS(const std::string& css) {
  web_contents()->InsertCSS(css);
}

bool WebContents::SavePage(const base::FilePath& full_file_path,
                           const content::SavePageType& save_type,
                           const SavePageHandler::SavePageCallback& callback) {
  auto handler = new SavePageHandler(web_contents(), callback);
  return handler->Handle(full_file_path, save_type);
}

void WebContents::ExecuteJavaScript(const base::string16& code,
                                    bool has_user_gesture) {
  Send(new AtomViewMsg_ExecuteJavaScript(routing_id(), code, has_user_gesture));
}

void WebContents::OpenDevTools(mate::Arguments* args) {
  if (type_ == REMOTE)
    return;

  bool detach = false;
  if (type_ == WEB_VIEW) {
    detach = true;
  } else if (args && args->Length() == 1) {
    mate::Dictionary options;
    args->GetNext(&options) && options.Get("detach", &detach);
  }
  managed_web_contents()->SetCanDock(!detach);
  managed_web_contents()->ShowDevTools();
}

void WebContents::CloseDevTools() {
  if (type_ == REMOTE)
    return;

  managed_web_contents()->CloseDevTools();
}

bool WebContents::IsDevToolsOpened() {
  if (type_ == REMOTE)
    return false;

  return managed_web_contents()->IsDevToolsViewShowing();
}

void WebContents::EnableDeviceEmulation(
    const blink::WebDeviceEmulationParams& params) {
  if (type_ == REMOTE)
    return;

  Send(new ViewMsg_EnableDeviceEmulation(routing_id(), params));
}

void WebContents::DisableDeviceEmulation() {
  if (type_ == REMOTE)
    return;

  Send(new ViewMsg_DisableDeviceEmulation(routing_id()));
}

void WebContents::ToggleDevTools() {
  if (IsDevToolsOpened())
    CloseDevTools();
  else
    OpenDevTools(nullptr);
}

void WebContents::InspectElement(int x, int y) {
  if (type_ == REMOTE)
    return;

  OpenDevTools(nullptr);
  scoped_refptr<content::DevToolsAgentHost> agent(
    content::DevToolsAgentHost::GetOrCreateFor(web_contents()));
  agent->InspectElement(x, y);
}

void WebContents::InspectServiceWorker() {
  if (type_ == REMOTE)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::TYPE_SERVICE_WORKER) {
      OpenDevTools(nullptr);
      managed_web_contents()->AttachTo(agent_host);
      break;
    }
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

void WebContents::AddWorkSpace(mate::Arguments* args,
                               const base::FilePath& path) {
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
  DevToolsAddFileSystem(path);
}

void WebContents::RemoveWorkSpace(mate::Arguments* args,
                                  const base::FilePath& path) {
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
  DevToolsRemoveFileSystem(path);
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

void WebContents::Focus() {
  web_contents()->Focus();
}

void WebContents::TabTraverse(bool reverse) {
  web_contents()->FocusThroughTabTraversal(reverse);
}

bool WebContents::SendIPCMessage(const base::string16& channel,
                                 const base::ListValue& args) {
  return Send(new AtomViewMsg_Message(routing_id(), channel, args));
}

void WebContents::SendInputEvent(v8::Isolate* isolate,
                                 v8::Local<v8::Value> input_event) {
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return;
  const auto host = view->GetRenderWidgetHost();
  if (!host)
    return;

  int type = mate::GetWebInputEventType(isolate, input_event);
  if (blink::WebInputEvent::isMouseEventType(type)) {
    blink::WebMouseEvent mouse_event;
    if (mate::ConvertFromV8(isolate, input_event, &mouse_event)) {
      host->ForwardMouseEvent(mouse_event);
      return;
    }
  } else if (blink::WebInputEvent::isKeyboardEventType(type)) {
    content::NativeWebKeyboardEvent keyboard_event;;
    if (mate::ConvertFromV8(isolate, input_event, &keyboard_event)) {
      host->ForwardKeyboardEvent(keyboard_event);
      return;
    }
  } else if (type == blink::WebInputEvent::MouseWheel) {
    blink::WebMouseWheelEvent mouse_wheel_event;
    if (mate::ConvertFromV8(isolate, input_event, &mouse_wheel_event)) {
      host->ForwardWheelEvent(mouse_wheel_event);
      return;
    }
  }

  isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
      isolate, "Invalid event object")));
}

void WebContents::BeginFrameSubscription(
    const FrameSubscriber::FrameCaptureCallback& callback) {
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view) {
    scoped_ptr<FrameSubscriber> frame_subscriber(new FrameSubscriber(
        isolate(), view->GetVisibleViewportSize(), callback));
    view->BeginFrameSubscription(frame_subscriber.Pass());
  }
}

void WebContents::EndFrameSubscription() {
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->EndFrameSubscription();
}

void WebContents::SetSize(const SetSizeParams& params) {
  if (guest_delegate_)
    guest_delegate_->SetSize(params);
}

void WebContents::SetAllowTransparency(bool allow) {
  if (guest_delegate_)
    guest_delegate_->SetAllowTransparency(allow);
}

bool WebContents::IsGuest() const {
  return type_ == WEB_VIEW;
}

v8::Local<v8::Value> WebContents::GetWebPreferences(v8::Isolate* isolate) {
  WebContentsPreferences* web_preferences =
      WebContentsPreferences::FromWebContents(web_contents());
  return mate::ConvertToV8(isolate, *web_preferences->web_preferences());
}

v8::Local<v8::Value> WebContents::GetOwnerBrowserWindow() {
  if (owner_window())
    return Window::From(isolate(), owner_window());
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebContents::Session(v8::Isolate* isolate) {
  return v8::Local<v8::Value>::New(isolate, session_);
}

v8::Local<v8::Value> WebContents::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

mate::ObjectTemplateBuilder WebContents::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  if (template_.IsEmpty())
    template_.Reset(isolate, mate::ObjectTemplateBuilder(isolate)
        .SetMethod("destroy", &WebContents::Destroy, true)
        .SetMethod("isAlive", &WebContents::IsAlive, true)
        .SetMethod("getId", &WebContents::GetID)
        .SetMethod("equal", &WebContents::Equal)
        .SetMethod("_loadUrl", &WebContents::LoadURL)
        .SetMethod("_getUrl", &WebContents::GetURL)
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
        .SetMethod("getUserAgent", &WebContents::GetUserAgent)
        .SetMethod("insertCSS", &WebContents::InsertCSS)
        .SetMethod("savePage", &WebContents::SavePage)
        .SetMethod("_executeJavaScript", &WebContents::ExecuteJavaScript)
        .SetMethod("openDevTools", &WebContents::OpenDevTools)
        .SetMethod("closeDevTools", &WebContents::CloseDevTools)
        .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
        .SetMethod("enableDeviceEmulation",
                   &WebContents::EnableDeviceEmulation)
        .SetMethod("disableDeviceEmulation",
                   &WebContents::DisableDeviceEmulation)
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
        .SetMethod("focus", &WebContents::Focus)
        .SetMethod("tabTraverse", &WebContents::TabTraverse)
        .SetMethod("_send", &WebContents::SendIPCMessage, true)
        .SetMethod("sendInputEvent", &WebContents::SendInputEvent)
        .SetMethod("beginFrameSubscription",
                   &WebContents::BeginFrameSubscription)
        .SetMethod("endFrameSubscription", &WebContents::EndFrameSubscription)
        .SetMethod("setSize", &WebContents::SetSize)
        .SetMethod("setAllowTransparency", &WebContents::SetAllowTransparency)
        .SetMethod("isGuest", &WebContents::IsGuest)
        .SetMethod("getWebPreferences", &WebContents::GetWebPreferences)
        .SetMethod("getOwnerBrowserWindow", &WebContents::GetOwnerBrowserWindow)
        .SetMethod("hasServiceWorker", &WebContents::HasServiceWorker)
        .SetMethod("unregisterServiceWorker",
                   &WebContents::UnregisterServiceWorker)
        .SetMethod("inspectServiceWorker", &WebContents::InspectServiceWorker)
        .SetMethod("print", &WebContents::Print)
        .SetMethod("_printToPDF", &WebContents::PrintToPDF)
        .SetMethod("addWorkSpace", &WebContents::AddWorkSpace)
        .SetMethod("removeWorkSpace", &WebContents::RemoveWorkSpace)
        .SetProperty("session", &WebContents::Session, true)
        .SetProperty("devToolsWebContents",
                     &WebContents::DevToolsWebContents, true)
        .Build());

  return mate::ObjectTemplateBuilder(
      isolate, v8::Local<v8::ObjectTemplate>::New(isolate, template_));
}

bool WebContents::IsDestroyed() const {
  return !IsAlive();
}

AtomBrowserContext* WebContents::GetBrowserContext() const {
  return static_cast<AtomBrowserContext*>(web_contents()->GetBrowserContext());
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

void WebContents::OnZoomLevelChanged(double level) {
  auto manager = web_contents()->GetBrowserContext()->GetGuestManager();
  if (!manager)
    return;
  manager->ForEachGuest(web_contents(),
                        base::Bind(&NotifyZoomLevelChanged,
                                   level));
}

// static
mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate, content::WebContents* web_contents) {
  // We have an existing WebContents object in JS.
  auto existing = TrackableObject::FromWrappedClass(isolate, web_contents);
  if (existing)
    return mate::CreateHandle(isolate, static_cast<WebContents*>(existing));

  // Otherwise create a new WebContents wrapper object.
  auto handle = mate::CreateHandle(isolate, new WebContents(web_contents));
  g_wrap_web_contents.Run(handle.ToV8());
  return handle;
}

// static
mate::Handle<WebContents> WebContents::Create(
    v8::Isolate* isolate, const mate::Dictionary& options) {
  auto handle = mate::CreateHandle(isolate, new WebContents(isolate, options));
  g_wrap_web_contents.Run(handle.ToV8());
  return handle;
}

void ClearWrapWebContents() {
  g_wrap_web_contents.Reset();
}

void SetWrapWebContents(const WrapWebContentsCallback& callback) {
  g_wrap_web_contents = callback;

  // Cleanup the wrapper on exit.
  atom::AtomBrowserMainParts::Get()->RegisterDestructionCallback(
      base::Bind(ClearWrapWebContents));
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
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_web_contents, Initialize)
