// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include <set>
#include <string>

#include "atom/browser/api/atom_api_debugger.h"
#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/api/atom_api_window.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_security_state_model_client.h"
#include "atom/browser/lib/bluetooth_chooser.h"
#include "atom/browser/native_window.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/web_view_guest_delegate.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/color_util.h"
#include "atom/common/mouse_util.h"
#include "atom/common/native_mate_converters/blink_converter.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/common/view_messages.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
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
#include "third_party/WebKit/public/web/WebFindOptions.h"
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
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        if (response_headers.HasKey(key)) {
          base::ListValue* values = nullptr;
          if (response_headers.GetList(key, &values))
            values->AppendString(value);
        } else {
          std::unique_ptr<base::ListValue> values(new base::ListValue());
          values->AppendString(value);
          response_headers.Set(key, std::move(values));
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
    save_type = base::ToLowerASCII(save_type);
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

template<>
struct Converter<atom::api::WebContents::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   atom::api::WebContents::Type val) {
    using Type = atom::api::WebContents::Type;
    std::string type = "";
    switch (val) {
      case Type::BACKGROUND_PAGE: type = "backgroundPage"; break;
      case Type::BROWSER_WINDOW: type = "window"; break;
      case Type::REMOTE: type = "remote"; break;
      case Type::WEB_VIEW: type = "webview"; break;
      default: break;
    }
    return mate::ConvertToV8(isolate, type);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     atom::api::WebContents::Type* out) {
    using Type = atom::api::WebContents::Type;
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "webview") {
      *out = Type::WEB_VIEW;
    } else if (type == "backgroundPage") {
      *out = Type::BACKGROUND_PAGE;
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

WebContents::WebContents(v8::Isolate* isolate,
                         content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      embedder_(nullptr),
      type_(REMOTE),
      request_id_(0),
      background_throttling_(true) {
  web_contents->SetUserAgentOverride(GetBrowserContext()->GetUserAgent());

  Init(isolate);
  AttachAsUserData(web_contents);
}

WebContents::WebContents(v8::Isolate* isolate,
                         const mate::Dictionary& options)
    : embedder_(nullptr),
      type_(BROWSER_WINDOW),
      request_id_(0),
      background_throttling_(true) {
  // Read options.
  options.Get("backgroundThrottling", &background_throttling_);

  // FIXME(zcbenz): We should read "type" parameter for better design, but
  // on Windows we have encountered a compiler bug that if we read "type"
  // from |options| and then set |type_|, a memory corruption will happen
  // and Electron will soon crash.
  // Remvoe this after we upgraded to use VS 2015 Update 3.
  bool b = false;
  if (options.Get("isGuest", &b) && b)
    type_ = WEB_VIEW;
  else if (options.Get("isBackgroundPage", &b) && b)
    type_ = BACKGROUND_PAGE;

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
  if (IsGuest()) {
    scoped_refptr<content::SiteInstance> site_instance =
        content::SiteInstance::CreateForURL(
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
  InitWithWebContents(web_contents, session->browser_context());

  managed_web_contents()->GetView()->SetDelegate(this);

  // Save the preferences in C++.
  new WebContentsPreferences(web_contents, options);

  // Intialize permission helper.
  WebContentsPermissionHelper::CreateForWebContents(web_contents);
  // Intialize security state client.
  AtomSecurityStateModelClient::CreateForWebContents(web_contents);

  web_contents->SetUserAgentOverride(GetBrowserContext()->GetUserAgent());

  if (IsGuest()) {
    guest_delegate_->Initialize(this);

    NativeWindow* owner_window = nullptr;
    if (options.Get("embedder", &embedder_) && embedder_) {
      // New WebContents's owner_window is the embedder's owner_window.
      auto relay =
          NativeWindowRelay::FromWebContents(embedder_->web_contents());
      if (relay)
        owner_window = relay->window.get();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);
  }

  Init(isolate);
  AttachAsUserData(web_contents);
}

WebContents::~WebContents() {
  // The destroy() is called.
  if (managed_web_contents()) {
    // For webview we need to tell content module to do some cleanup work before
    // destroying it.
    if (type_ == WEB_VIEW)
      guest_delegate_->Destroy();

    // The WebContentsDestroyed will not be called automatically because we
    // unsubscribe from webContents before destroying it. So we have to manually
    // call it here to make sure "destroyed" event is emitted.
    RenderViewDeleted(web_contents()->GetRenderViewHost());
    WebContentsDestroyed();
  }
}

bool WebContents::AddMessageToConsole(content::WebContents* source,
                                      int32_t level,
                                      const base::string16& message,
                                      int32_t line_no,
                                      const base::string16& source_id) {
  if (type_ == BROWSER_WINDOW) {
    return false;
  } else {
    Emit("console-message", level, message, line_no, source_id);
    return true;
  }
}

void WebContents::OnCreateWindow(const GURL& target_url,
                                 const std::string& frame_name,
                                 WindowOpenDisposition disposition) {
  if (type_ == BROWSER_WINDOW)
    Emit("-new-window", target_url, frame_name, disposition);
  else
    Emit("new-window", target_url, frame_name, disposition);
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
  if (type_ == BROWSER_WINDOW && owner_window())
    owner_window()->CloseContents(source);
}

void WebContents::ActivateContents(content::WebContents* source) {
  Emit("activate");
}

void WebContents::UpdateTargetURL(content::WebContents* source,
                                  const GURL& url) {
  Emit("update-target-url", url);
}

bool WebContents::IsPopupOrPanel(const content::WebContents* source) const {
  return type_ == BROWSER_WINDOW;
}

void WebContents::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (type_ == WEB_VIEW && embedder_) {
    // Send the unhandled keyboard events back to the embedder.
    embedder_->HandleKeyboardEvent(source, event);
  } else {
    // Go to the default keyboard handling.
    CommonWebContentsDelegate::HandleKeyboardEvent(source, event);
  }
}

void WebContents::EnterFullscreenModeForTab(content::WebContents* source,
                                            const GURL& origin) {
  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(source);
  auto callback = base::Bind(&WebContents::OnEnterFullscreenModeForTab,
                             base::Unretained(this), source, origin);
  permission_helper->RequestFullscreenPermission(callback);
}

void WebContents::OnEnterFullscreenModeForTab(content::WebContents* source,
                                              const GURL& origin,
                                              bool allowed) {
  if (!allowed)
    return;
  CommonWebContentsDelegate::EnterFullscreenModeForTab(source, origin);
  Emit("enter-html-full-screen");
}

void WebContents::ExitFullscreenModeForTab(content::WebContents* source) {
  CommonWebContentsDelegate::ExitFullscreenModeForTab(source);
  Emit("leave-html-full-screen");
}

void WebContents::RendererUnresponsive(content::WebContents* source) {
  Emit("unresponsive");
  if (type_ == BROWSER_WINDOW && owner_window())
    owner_window()->RendererUnresponsive(source);
}

void WebContents::RendererResponsive(content::WebContents* source) {
  Emit("responsive");
  if (type_ == BROWSER_WINDOW && owner_window())
    owner_window()->RendererResponsive(source);
}

bool WebContents::HandleContextMenu(const content::ContextMenuParams& params) {
  if (params.custom_context.is_pepper_menu) {
    Emit("pepper-context-menu", std::make_pair(params, web_contents()));
    web_contents()->NotifyContextMenuClosed(params.custom_context);
  } else {
    Emit("context-menu", std::make_pair(params, web_contents()));
  }

  return true;
}

bool WebContents::OnGoToEntryOffset(int offset) {
  GoToOffset(offset);
  return false;
}

void WebContents::FindReply(content::WebContents* web_contents,
                            int request_id,
                            int number_of_matches,
                            const gfx::Rect& selection_rect,
                            int active_match_ordinal,
                            bool final_update) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());

  mate::Dictionary result = mate::Dictionary::CreateEmpty(isolate());
  if (number_of_matches == -1) {
    result.Set("requestId", request_id);
    result.Set("selectionArea", selection_rect);
    result.Set("finalUpdate", final_update);
    result.Set("activeMatchOrdinal", active_match_ordinal);
    Emit("found-in-page", result);
  } else if (final_update) {
    result.Set("requestId", request_id);
    result.Set("matches", number_of_matches);
    result.Set("finalUpdate", final_update);
    Emit("found-in-page", result);
  }
}

bool WebContents::CheckMediaAccessPermission(
    content::WebContents* web_contents,
    const GURL& security_origin,
    content::MediaStreamType type) {
  return true;
}

void WebContents::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestMediaAccessPermission(request, callback);
}

void WebContents::RequestToLockMouse(
    content::WebContents* web_contents,
    bool user_gesture,
    bool last_unlocked_by_target) {
  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestPointerLockPermission(user_gesture);
}

std::unique_ptr<content::BluetoothChooser> WebContents::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
  std::unique_ptr<BluetoothChooser> bluetooth_chooser(
      new BluetoothChooser(this, event_handler));
  return std::move(bluetooth_chooser);
}

void WebContents::BeforeUnloadFired(const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
}

void WebContents::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  Emit("render-view-deleted", render_view_host->GetProcess()->GetID());
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

void WebContents::MediaStartedPlaying(const MediaPlayerId& id) {
  Emit("media-started-playing");
}

void WebContents::MediaStoppedPlaying(const MediaPlayerId& id) {
  Emit("media-paused");
}

void WebContents::DidChangeThemeColor(SkColor theme_color) {
  std::string hex_theme_color = base::StringPrintf("#%02X%02X%02X",
    SkColorGetR(theme_color),
    SkColorGetG(theme_color),
    SkColorGetB(theme_color));
  Emit("did-change-theme-color", hex_theme_color);
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

void WebContents::DidFailProvisionalLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& url,
    int code,
    const base::string16& description,
    bool was_ignored_by_handler) {
  bool is_main_frame = !render_frame_host->GetParent();
  Emit("did-fail-provisional-load", code, description, url, is_main_frame);
  Emit("did-fail-load", code, description, url, is_main_frame);
}

void WebContents::DidFailLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& url,
                              int error_code,
                              const base::string16& error_description,
                              bool was_ignored_by_handler) {
  bool is_main_frame = !render_frame_host->GetParent();
  Emit("did-fail-load", error_code, error_description, url, is_main_frame);
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
       details.headers.get(),
       ResourceTypeToString(details.resource_type));
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
    Emit("did-navigate", params.url);
  else if (details.is_in_page)
    Emit("did-navigate-in-page", params.url);
}

void WebContents::TitleWasSet(content::NavigationEntry* entry,
                              bool explicit_set) {
  if (entry)
    Emit("-page-title-updated", entry->GetTitle(), explicit_set);
  else
    Emit("-page-title-updated", "", explicit_set);
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

void WebContents::DevToolsReloadPage() {
  Emit("devtools-reload-page");
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

  // Set inspected tabID.
  base::FundamentalValue tab_id(ID());
  managed_web_contents()->CallClientFunction(
      "DevToolsAPI.setInspectedTabId", &tab_id, nullptr, nullptr);

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
    IPC_MESSAGE_HANDLER_CODE(ViewHostMsg_SetCursor, OnCursorChange,
      handled = false)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

// There are three ways of destroying a webContents:
// 1. call webContents.destory();
// 2. garbage collection;
// 3. user closes the window of webContents;
// For webview only #1 will happen, for BrowserWindow both #1 and #3 may
// happen. The #2 should never happen for webContents, because webview is
// managed by GuestViewManager, and BrowserWindow's webContents is managed
// by api::Window.
// For #1, the destructor will do the cleanup work and we only need to make
// sure "destroyed" event is emitted. For #3, the content::WebContents will
// be destroyed on close, and WebContentsDestroyed would be called for it, so
// we need to make sure the api::WebContents is also deleted.
void WebContents::WebContentsDestroyed() {
  // This event is only for internal use, which is emitted when WebContents is
  // being destroyed.
  Emit("will-destroy");

  // Cleanup relationships with other parts.
  RemoveFromWeakMap();

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();

  Emit("destroyed");

  // Destroy the native class in next tick.
  base::MessageLoop::current()->PostTask(FROM_HERE, GetDestroyClosure());
}

void WebContents::NavigationEntryCommitted(
    const content::LoadCommittedDetails& details) {
  Emit("navigation-entry-commited", details.entry->GetURL(),
       details.is_in_page, details.did_replace_entry);
}

int WebContents::GetID() const {
  return web_contents()->GetRenderProcessHost()->GetID();
}

WebContents::Type WebContents::GetType() const {
  return type_;
}

bool WebContents::Equal(const WebContents* web_contents) const {
  return GetID() == web_contents->GetID();
}

void WebContents::LoadURL(const GURL& url, const mate::Dictionary& options) {
  if (!url.is_valid()) {
    Emit("did-fail-load",
         static_cast<int>(net::ERR_INVALID_URL),
         net::ErrorToShortString(net::ERR_INVALID_URL),
         url.possibly_invalid_spec(),
         true);
    return;
  }

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

  // Set the background color of RenderWidgetHostView.
  // We have to call it right after LoadURL because the RenderViewHost is only
  // created after loading a page.
  const auto view = web_contents()->GetRenderWidgetHostView();
  WebContentsPreferences* web_preferences =
      WebContentsPreferences::FromWebContents(web_contents());
  std::string color_name;
  if (web_preferences->web_preferences()->GetString(options::kBackgroundColor,
                                                    &color_name)) {
    view->SetBackgroundColor(ParseHexColor(color_name));
  } else {
    view->SetBackgroundColor(SK_ColorTRANSPARENT);
  }

  // For the same reason we can only disable hidden here.
  const auto host = static_cast<content::RenderWidgetHostImpl*>(
      view->GetRenderWidgetHost());
  host->disable_hidden_ = !background_throttling_;
}

void WebContents::DownloadURL(const GURL& url) {
  auto browser_context = web_contents()->GetBrowserContext();
  auto download_manager =
    content::BrowserContext::GetDownloadManager(browser_context);

  download_manager->DownloadUrl(
    content::DownloadUrlParameters::FromWebContents(web_contents(), url));
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

bool WebContents::IsLoadingMainFrame() const {
  // Comparing site instances works because Electron always creates a new site
  // instance when navigating, regardless of origin. See AtomBrowserClient.
  return (web_contents()->GetLastCommittedURL().is_empty() ||
          web_contents()->GetSiteInstance() !=
          web_contents()->GetPendingSiteInstance()) && IsLoading();
}

bool WebContents::IsWaitingForResponse() const {
  return web_contents()->IsWaitingForResponse();
}

void WebContents::Stop() {
  web_contents()->Stop();
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

void WebContents::OpenDevTools(mate::Arguments* args) {
  if (type_ == REMOTE)
    return;

  std::string state;
  if (type_ == WEB_VIEW || !owner_window()) {
    state = "detach";
  } else if (args && args->Length() == 1) {
    bool detach = false;
    mate::Dictionary options;
    if (args->GetNext(&options)) {
      options.Get("mode", &state);
      options.Get("detach", &detach);
      if (state.empty() && detach)
        state = "detach";
    }
  }
  managed_web_contents()->SetDockState(state);
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

bool WebContents::IsDevToolsFocused() {
  if (type_ == REMOTE)
    return false;

  return managed_web_contents()->GetView()->IsDevToolsViewFocused();
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

uint32_t WebContents::FindInPage(mate::Arguments* args) {
  uint32_t request_id = GetNextRequestId();
  base::string16 search_text;
  blink::WebFindOptions options;
  if (!args->GetNext(&search_text) || search_text.empty()) {
    args->ThrowError("Must provide a non-empty search content");
    return 0;
  }

  args->GetNext(&options);

  web_contents()->Find(request_id, search_text, options);
  return request_id;
}

void WebContents::StopFindInPage(content::StopFindAction action) {
  web_contents()->StopFinding(action);
}

void WebContents::ShowDefinitionForSelection() {
#if defined(OS_MACOSX)
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->ShowDefinitionForSelection();
#endif
}

void WebContents::Focus() {
  web_contents()->Focus();
}

void WebContents::TabTraverse(bool reverse) {
  web_contents()->FocusThroughTabTraversal(reverse);
}

bool WebContents::SendIPCMessage(bool all_frames,
                                 const base::string16& channel,
                                 const base::ListValue& args) {
  return Send(new AtomViewMsg_Message(routing_id(), all_frames, channel, args));
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
    content::NativeWebKeyboardEvent keyboard_event;
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
    std::unique_ptr<FrameSubscriber> frame_subscriber(new FrameSubscriber(
        isolate(), view, callback));
    view->BeginFrameSubscription(std::move(frame_subscriber));
  }
}

void WebContents::EndFrameSubscription() {
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->EndFrameSubscription();
}

void WebContents::OnCursorChange(const content::WebCursor& cursor) {
  content::WebCursor::CursorInfo info;
  cursor.GetCursorInfo(&info);

  if (cursor.IsCustom()) {
    Emit("cursor-changed", CursorTypeToString(info),
      gfx::Image::CreateFrom1xBitmap(info.custom_image),
      info.image_scale_factor);
  } else {
    Emit("cursor-changed", CursorTypeToString(info));
  }
}

void WebContents::SetSize(const SetSizeParams& params) {
  if (guest_delegate_)
    guest_delegate_->SetSize(params);
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

int32_t WebContents::ID() const {
  return weak_map_id();
}

v8::Local<v8::Value> WebContents::Session(v8::Isolate* isolate) {
  return v8::Local<v8::Value>::New(isolate, session_);
}

content::WebContents* WebContents::HostWebContents() {
  if (!embedder_)
    return nullptr;
  return embedder_->web_contents();
}

v8::Local<v8::Value> WebContents::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

v8::Local<v8::Value> WebContents::Debugger(v8::Isolate* isolate) {
  if (debugger_.IsEmpty()) {
    auto handle = atom::api::Debugger::Create(isolate, web_contents());
    debugger_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, debugger_);
}

// static
void WebContents::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .MakeDestroyable()
      .SetMethod("getId", &WebContents::GetID)
      .SetMethod("equal", &WebContents::Equal)
      .SetMethod("_loadURL", &WebContents::LoadURL)
      .SetMethod("downloadURL", &WebContents::DownloadURL)
      .SetMethod("_getURL", &WebContents::GetURL)
      .SetMethod("getTitle", &WebContents::GetTitle)
      .SetMethod("isLoading", &WebContents::IsLoading)
      .SetMethod("isLoadingMainFrame", &WebContents::IsLoadingMainFrame)
      .SetMethod("isWaitingForResponse", &WebContents::IsWaitingForResponse)
      .SetMethod("_stop", &WebContents::Stop)
      .SetMethod("_goBack", &WebContents::GoBack)
      .SetMethod("_goForward", &WebContents::GoForward)
      .SetMethod("_goToOffset", &WebContents::GoToOffset)
      .SetMethod("isCrashed", &WebContents::IsCrashed)
      .SetMethod("setUserAgent", &WebContents::SetUserAgent)
      .SetMethod("getUserAgent", &WebContents::GetUserAgent)
      .SetMethod("insertCSS", &WebContents::InsertCSS)
      .SetMethod("savePage", &WebContents::SavePage)
      .SetMethod("openDevTools", &WebContents::OpenDevTools)
      .SetMethod("closeDevTools", &WebContents::CloseDevTools)
      .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
      .SetMethod("isDevToolsFocused", &WebContents::IsDevToolsFocused)
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
      .SetMethod("findInPage", &WebContents::FindInPage)
      .SetMethod("stopFindInPage", &WebContents::StopFindInPage)
      .SetMethod("focus", &WebContents::Focus)
      .SetMethod("tabTraverse", &WebContents::TabTraverse)
      .SetMethod("_send", &WebContents::SendIPCMessage)
      .SetMethod("sendInputEvent", &WebContents::SendInputEvent)
      .SetMethod("beginFrameSubscription",
                 &WebContents::BeginFrameSubscription)
      .SetMethod("endFrameSubscription", &WebContents::EndFrameSubscription)
      .SetMethod("setSize", &WebContents::SetSize)
      .SetMethod("isGuest", &WebContents::IsGuest)
      .SetMethod("getType", &WebContents::GetType)
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
      .SetMethod("showDefinitionForSelection",
                 &WebContents::ShowDefinitionForSelection)
      .SetProperty("id", &WebContents::ID)
      .SetProperty("session", &WebContents::Session)
      .SetProperty("hostWebContents", &WebContents::HostWebContents)
      .SetProperty("devToolsWebContents", &WebContents::DevToolsWebContents)
      .SetProperty("debugger", &WebContents::Debugger);
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

// static
mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate, content::WebContents* web_contents) {
  // We have an existing WebContents object in JS.
  auto existing = TrackableObject::FromWrappedClass(isolate, web_contents);
  if (existing)
    return mate::CreateHandle(isolate, static_cast<WebContents*>(existing));

  // Otherwise create a new WebContents wrapper object.
  auto handle = mate::CreateHandle(
      isolate, new WebContents(isolate, web_contents));
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

void SetWrapWebContents(const WrapWebContentsCallback& callback) {
  g_wrap_web_contents = callback;
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
  dict.SetMethod("fromId",
                 &mate::TrackableObject<atom::api::WebContents>::FromWeakMapID);
  dict.SetMethod("getAllWebContents",
                 &mate::TrackableObject<atom::api::WebContents>::GetAll);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_web_contents, Initialize)
