// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "atom/browser/api/atom_api_browser_window.h"
#include "atom/browser/api/atom_api_debugger.h"
#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_javascript_dialog_manager.h"
#include "atom/browser/atom_navigation_throttle.h"
#include "atom/browser/child_web_contents_tracker.h"
#include "atom/browser/lib/bluetooth_chooser.h"
#include "atom/browser/native_window.h"
#include "atom/browser/net/atom_network_delegate.h"
#if defined(ENABLE_OSR)
#include "atom/browser/osr/osr_output_device.h"
#include "atom/browser/osr/osr_render_widget_host_view.h"
#include "atom/browser/osr/osr_web_contents_view.h"
#endif
#include "atom/browser/ui/drag_util.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/web_contents_zoom_controller.h"
#include "atom/browser/web_view_guest_delegate.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/api/atom_api_native_image.h"
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
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/network_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_manager.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/common/view_messages.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/download_request_utils.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/context_menu_params.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/url_request/url_request_context.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/web/web_find_options.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"

#if !defined(OS_MACOSX)
#include "ui/aura/window.h"
#endif

#if defined(OS_LINUX) || defined(OS_WIN)
#include "content/public/common/renderer_preferences.h"
#include "ui/gfx/font_render_params.h"
#endif

#include "atom/common/node_includes.h"

namespace {

struct PrintSettings {
  bool silent;
  bool print_background;
  base::string16 device_name;
};
}  // namespace

namespace mate {

template <>
struct Converter<PrintSettings> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     PrintSettings* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    dict.Get("silent", &(out->silent));
    dict.Get("printBackground", &(out->print_background));
    dict.Get("deviceName", &(out->device_name));
    return true;
  }
};

template <>
struct Converter<printing::PrinterBasicInfo> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const printing::PrinterBasicInfo& val) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("name", val.printer_name);
    dict.Set("description", val.printer_description);
    dict.Set("status", val.printer_status);
    dict.Set("isDefault", val.is_default ? true : false);
    dict.Set("options", val.options);
    return dict.GetHandle();
  }
};

template <>
struct Converter<WindowOpenDisposition> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   WindowOpenDisposition val) {
    std::string disposition = "other";
    switch (val) {
      case WindowOpenDisposition::CURRENT_TAB:
        disposition = "default";
        break;
      case WindowOpenDisposition::NEW_FOREGROUND_TAB:
        disposition = "foreground-tab";
        break;
      case WindowOpenDisposition::NEW_BACKGROUND_TAB:
        disposition = "background-tab";
        break;
      case WindowOpenDisposition::NEW_POPUP:
      case WindowOpenDisposition::NEW_WINDOW:
        disposition = "new-window";
        break;
      case WindowOpenDisposition::SAVE_TO_DISK:
        disposition = "save-to-disk";
        break;
      default:
        break;
    }
    return mate::ConvertToV8(isolate, disposition);
  }
};

template <>
struct Converter<content::SavePageType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
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

template <>
struct Converter<atom::api::WebContents::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   atom::api::WebContents::Type val) {
    using Type = atom::api::WebContents::Type;
    std::string type = "";
    switch (val) {
      case Type::BACKGROUND_PAGE:
        type = "backgroundPage";
        break;
      case Type::BROWSER_WINDOW:
        type = "window";
        break;
      case Type::BROWSER_VIEW:
        type = "browserView";
        break;
      case Type::REMOTE:
        type = "remote";
        break;
      case Type::WEB_VIEW:
        type = "webview";
        break;
      case Type::OFF_SCREEN:
        type = "offscreen";
        break;
      default:
        break;
    }
    return mate::ConvertToV8(isolate, type);
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::api::WebContents::Type* out) {
    using Type = atom::api::WebContents::Type;
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "backgroundPage") {
      *out = Type::BACKGROUND_PAGE;
    } else if (type == "browserView") {
      *out = Type::BROWSER_VIEW;
    } else if (type == "webview") {
      *out = Type::WEB_VIEW;
#if defined(ENABLE_OSR)
    } else if (type == "offscreen") {
      *out = Type::OFF_SCREEN;
#endif
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

content::ServiceWorkerContext* GetServiceWorkerContext(
    const content::WebContents* web_contents) {
  auto* context = web_contents->GetBrowserContext();
  auto* site_instance = web_contents->GetSiteInstance();
  if (!context || !site_instance)
    return nullptr;

  auto* storage_partition =
      content::BrowserContext::GetStoragePartition(context, site_instance);
  if (!storage_partition)
    return nullptr;

  return storage_partition->GetServiceWorkerContext();
}

// Called when CapturePage is done.
void OnCapturePageDone(const base::Callback<void(const gfx::Image&)>& callback,
                       const SkBitmap& bitmap) {
  // Hack to enable transparency in captured image
  // TODO(nitsakh) Remove hack once fixed in chromium
  const_cast<SkBitmap&>(bitmap).setAlphaType(kPremul_SkAlphaType);
  callback.Run(gfx::Image::CreateFrom1xBitmap(bitmap));
}

}  // namespace

struct WebContents::FrameDispatchHelper {
  WebContents* api_web_contents;
  content::RenderFrameHost* rfh;

  bool Send(IPC::Message* msg) { return rfh->Send(msg); }

  void OnSetTemporaryZoomLevel(double level, IPC::Message* reply_msg) {
    api_web_contents->OnSetTemporaryZoomLevel(rfh, level, reply_msg);
  }

  void OnGetZoomLevel(IPC::Message* reply_msg) {
    api_web_contents->OnGetZoomLevel(rfh, reply_msg);
  }

  void OnRendererMessageSync(const std::string& channel,
                             const base::ListValue& args,
                             IPC::Message* message) {
    api_web_contents->OnRendererMessageSync(rfh, channel, args, message);
  }
};

WebContents::WebContents(v8::Isolate* isolate,
                         content::WebContents* web_contents,
                         Type type)
    : content::WebContentsObserver(web_contents), type_(type) {
  const mate::Dictionary options = mate::Dictionary::CreateEmpty(isolate);
  if (type == REMOTE) {
    web_contents->SetUserAgentOverride(GetBrowserContext()->GetUserAgent(),
                                       false);
    Init(isolate);
    AttachAsUserData(web_contents);
    InitZoomController(web_contents, options);
  } else {
    auto session = Session::CreateFrom(isolate, GetBrowserContext());
    session_.Reset(isolate, session.ToV8());
    InitWithSessionAndOptions(isolate, web_contents, session, options);
  }
}

WebContents::WebContents(v8::Isolate* isolate,
                         const mate::Dictionary& options) {
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
  else if (options.Get("isBrowserView", &b) && b)
    type_ = BROWSER_VIEW;
#if defined(ENABLE_OSR)
  else if (options.Get(options::kOffscreen, &b) && b)
    type_ = OFF_SCREEN;
#endif

  // Init embedder earlier
  options.Get("embedder", &embedder_);

  // Whether to enable DevTools.
  options.Get("devTools", &enable_devtools_);

  // Obtain the session.
  std::string partition;
  mate::Handle<api::Session> session;
  if (options.Get("session", &session) && !session.IsEmpty()) {
  } else if (options.Get("partition", &partition)) {
    session = Session::FromPartition(isolate, partition);
  } else {
    // Use the default session if not specified.
    session = Session::FromPartition(isolate, "");
  }
  session_.Reset(isolate, session.ToV8());

  content::WebContents* web_contents;
  if (IsGuest()) {
    scoped_refptr<content::SiteInstance> site_instance =
        content::SiteInstance::CreateForURL(session->browser_context(),
                                            GURL("chrome-guest://fake-host"));
    content::WebContents::CreateParams params(session->browser_context(),
                                              site_instance);
    guest_delegate_.reset(
        new WebViewGuestDelegate(embedder_->web_contents(), this));
    params.guest_delegate = guest_delegate_.get();

#if defined(ENABLE_OSR)
    if (embedder_ && embedder_->IsOffScreen()) {
      auto* view = new OffScreenWebContentsView(
          false, base::Bind(&WebContents::OnPaint, base::Unretained(this)));
      params.view = view;
      params.delegate_view = view;

      web_contents = content::WebContents::Create(params);
      view->SetWebContents(web_contents);
    } else {
#endif
      web_contents = content::WebContents::Create(params);
#if defined(ENABLE_OSR)
    }
  } else if (IsOffScreen()) {
    bool transparent = false;
    options.Get("transparent", &transparent);

    content::WebContents::CreateParams params(session->browser_context());
    auto* view = new OffScreenWebContentsView(
        transparent, base::Bind(&WebContents::OnPaint, base::Unretained(this)));
    params.view = view;
    params.delegate_view = view;

    web_contents = content::WebContents::Create(params);
    view->SetWebContents(web_contents);
#endif
  } else {
    content::WebContents::CreateParams params(session->browser_context());
    web_contents = content::WebContents::Create(params);
  }

  InitWithSessionAndOptions(isolate, web_contents, session, options);
}

void WebContents::InitZoomController(content::WebContents* web_contents,
                                     const mate::Dictionary& options) {
  WebContentsZoomController::CreateForWebContents(web_contents);
  zoom_controller_ = WebContentsZoomController::FromWebContents(web_contents);
  double zoom_factor;
  if (options.Get(options::kZoomFactor, &zoom_factor))
    zoom_controller_->SetDefaultZoomFactor(zoom_factor);
}

void WebContents::InitWithSessionAndOptions(v8::Isolate* isolate,
                                            content::WebContents* web_contents,
                                            mate::Handle<api::Session> session,
                                            const mate::Dictionary& options) {
  Observe(web_contents);
  InitWithWebContents(web_contents, session->browser_context(), IsGuest());

  managed_web_contents()->GetView()->SetDelegate(this);

  auto* prefs = web_contents->GetMutableRendererPrefs();
  prefs->accept_languages = g_browser_process->GetApplicationLocale();

#if defined(OS_LINUX) || defined(OS_WIN)
  // Update font settings.
  CR_DEFINE_STATIC_LOCAL(
      const gfx::FontRenderParams, params,
      (gfx::GetFontRenderParams(gfx::FontRenderParamsQuery(), nullptr)));
  prefs->should_antialias_text = params.antialiasing;
  prefs->use_subpixel_positioning = params.subpixel_positioning;
  prefs->hinting = params.hinting;
  prefs->use_autohinter = params.autohinter;
  prefs->use_bitmaps = params.use_bitmaps;
  prefs->subpixel_rendering = params.subpixel_rendering;
#endif

  // Save the preferences in C++.
  new WebContentsPreferences(web_contents, options);

  // Initialize permission helper.
  WebContentsPermissionHelper::CreateForWebContents(web_contents);
  // Initialize security state client.
  SecurityStateTabHelper::CreateForWebContents(web_contents);
  // Initialize zoom controller.
  InitZoomController(web_contents, options);

  web_contents->SetUserAgentOverride(GetBrowserContext()->GetUserAgent(),
                                     false);

  if (IsGuest()) {
    NativeWindow* owner_window = nullptr;
    if (embedder_) {
      // New WebContents's owner_window is the embedder's owner_window.
      auto* relay =
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
    managed_web_contents()->GetView()->SetDelegate(nullptr);

    RenderViewDeleted(web_contents()->GetRenderViewHost());

    if (type_ == WEB_VIEW) {
      DestroyWebContents(false /* async */);
    } else {
      if (type_ == BROWSER_WINDOW && owner_window()) {
        for (ExtendedWebContentsObserver& observer : observers_)
          observer.OnCloseContents();
      } else {
        DestroyWebContents(true /* async */);
      }
      // The WebContentsDestroyed will not be called automatically because we
      // destroy the webContents in the next tick. So we have to manually
      // call it here to make sure "destroyed" event is emitted.
      WebContentsDestroyed();
    }
  }
}

void WebContents::DestroyWebContents(bool async) {
  // This event is only for internal use, which is emitted when WebContents is
  // being destroyed.
  Emit("will-destroy");
  ResetManagedWebContents(async);
}

bool WebContents::DidAddMessageToConsole(content::WebContents* source,
                                         int32_t level,
                                         const base::string16& message,
                                         int32_t line_no,
                                         const base::string16& source_id) {
  return Emit("console-message", level, message, line_no, source_id);
}

void WebContents::OnCreateWindow(
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const std::vector<std::string>& features,
    const scoped_refptr<network::ResourceRequestBody>& body) {
  if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
    Emit("-new-window", target_url, frame_name, disposition, features, body,
         referrer);
  else
    Emit("new-window", target_url, frame_name, disposition, features);
}

void WebContents::WebContentsCreated(content::WebContents* source_contents,
                                     int opener_render_process_id,
                                     int opener_render_frame_id,
                                     const std::string& frame_name,
                                     const GURL& target_url,
                                     content::WebContents* new_contents) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto api_web_contents = CreateFrom(isolate(), new_contents, BROWSER_WINDOW);
  Emit("-web-contents-created", api_web_contents, target_url, frame_name);
}

void WebContents::AddNewContents(content::WebContents* source,
                                 content::WebContents* new_contents,
                                 WindowOpenDisposition disposition,
                                 const gfx::Rect& initial_rect,
                                 bool user_gesture,
                                 bool* was_blocked) {
  new ChildWebContentsTracker(new_contents);
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto api_web_contents = CreateFrom(isolate(), new_contents);
  if (Emit("-add-new-contents", api_web_contents, disposition, user_gesture,
           initial_rect.x(), initial_rect.y(), initial_rect.width(),
           initial_rect.height())) {
    api_web_contents->DestroyWebContents(true /* async */);
  }
}

content::WebContents* WebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (params.disposition != WindowOpenDisposition::CURRENT_TAB) {
    if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
      Emit("-new-window", params.url, "", params.disposition);
    else
      Emit("new-window", params.url, "", params.disposition);
    return nullptr;
  }

  // Give user a chance to cancel navigation.
  if (Emit("will-navigate", params.url))
    return nullptr;

  // Don't load the URL if the web contents was marked as destroyed from a
  // will-navigate event listener
  if (IsDestroyed())
    return nullptr;

  return CommonWebContentsDelegate::OpenURLFromTab(source, params);
}

void WebContents::BeforeUnloadFired(content::WebContents* tab,
                                    bool proceed,
                                    bool* proceed_to_fire_unload) {
  if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
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
#if defined(TOOLKIT_VIEWS) && !defined(OS_MACOSX)
  HideAutofillPopup();
#endif
  if (managed_web_contents())
    managed_web_contents()->GetView()->SetDelegate(nullptr);
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnCloseContents();
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

content::KeyboardEventProcessingResult WebContents::PreHandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (event.GetType() == blink::WebInputEvent::Type::kRawKeyDown ||
      event.GetType() == blink::WebInputEvent::Type::kKeyUp) {
    bool prevent_default = Emit("before-input-event", event);
    if (prevent_default) {
      return content::KeyboardEventProcessingResult::HANDLED;
    }
  }

  return content::KeyboardEventProcessingResult::NOT_HANDLED;
}

void WebContents::EnterFullscreenModeForTab(content::WebContents* source,
                                            const GURL& origin) {
  auto* permission_helper =
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

void WebContents::RendererUnresponsive(
    content::WebContents* source,
    content::RenderWidgetHost* render_widget_host) {
  Emit("unresponsive");
}

void WebContents::RendererResponsive(
    content::WebContents* source,
    content::RenderWidgetHost* render_widget_host) {
  Emit("responsive");
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnRendererResponsive();
}

bool WebContents::HandleContextMenu(const content::ContextMenuParams& params) {
  if (params.custom_context.is_pepper_menu) {
    Emit("pepper-context-menu", std::make_pair(params, web_contents()),
         base::Bind(&content::WebContents::NotifyContextMenuClosed,
                    base::Unretained(web_contents()), params.custom_context));
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
  if (!final_update)
    return;

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  mate::Dictionary result = mate::Dictionary::CreateEmpty(isolate());
  result.Set("requestId", request_id);
  result.Set("matches", number_of_matches);
  result.Set("selectionArea", selection_rect);
  result.Set("activeMatchOrdinal", active_match_ordinal);
  result.Set("finalUpdate", final_update);  // Deprecate after 2.0
  Emit("found-in-page", result);
}

bool WebContents::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    content::MediaStreamType type) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  return permission_helper->CheckMediaAccessPermission(security_origin, type);
}

void WebContents::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestMediaAccessPermission(request, callback);
}

void WebContents::RequestToLockMouse(content::WebContents* web_contents,
                                     bool user_gesture,
                                     bool last_unlocked_by_target) {
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestPointerLockPermission(user_gesture);
}

std::unique_ptr<content::BluetoothChooser> WebContents::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
  return std::make_unique<BluetoothChooser>(this, event_handler);
}

content::JavaScriptDialogManager* WebContents::GetJavaScriptDialogManager(
    content::WebContents* source) {
  if (!dialog_manager_)
    dialog_manager_.reset(new AtomJavaScriptDialogManager(this));

  return dialog_manager_.get();
}

void WebContents::OnAudioStateChanged(content::WebContents* web_contents,
                                      bool audible) {
  Emit("-audio-state-changed", audible);
}

void WebContents::BeforeUnloadFired(const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
}

void WebContents::RenderViewCreated(content::RenderViewHost* render_view_host) {
  auto* const impl = content::RenderWidgetHostImpl::FromID(
      render_view_host->GetProcess()->GetID(),
      render_view_host->GetRoutingID());
  if (impl)
    impl->disable_hidden_ = !background_throttling_;
}

void WebContents::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  Emit("render-view-deleted", render_view_host->GetProcess()->GetID());
}

void WebContents::RenderProcessGone(base::TerminationStatus status) {
  Emit("crashed", status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED);
}

void WebContents::PluginCrashed(const base::FilePath& plugin_path,
                                base::ProcessId plugin_pid) {
  content::WebPluginInfo info;
  auto* plugin_service = content::PluginService::GetInstance();
  plugin_service->GetPluginInfoByPath(plugin_path, &info);
  Emit("plugin-crashed", info.name, info.version);
}

void WebContents::MediaStartedPlaying(const MediaPlayerInfo& video_type,
                                      const MediaPlayerId& id) {
  Emit("media-started-playing");
}

void WebContents::MediaStoppedPlaying(
    const MediaPlayerInfo& video_type,
    const MediaPlayerId& id,
    content::WebContentsObserver::MediaStoppedReason reason) {
  Emit("media-paused");
}

void WebContents::DidChangeThemeColor(SkColor theme_color) {
  if (theme_color != SK_ColorTRANSPARENT) {
    Emit("did-change-theme-color", atom::ToRGBHex(theme_color));
  } else {
    Emit("did-change-theme-color", nullptr);
  }
}

void WebContents::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host->GetParent())
    Emit("dom-ready");
}

void WebContents::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                const GURL& validated_url) {
  bool is_main_frame = !render_frame_host->GetParent();
  int frame_process_id = render_frame_host->GetProcess()->GetID();
  int frame_routing_id = render_frame_host->GetRoutingID();
  Emit("did-frame-finish-load", is_main_frame, frame_process_id,
       frame_routing_id);

  if (is_main_frame)
    Emit("did-finish-load");
}

void WebContents::DidFailLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& url,
                              int error_code,
                              const base::string16& error_description) {
  bool is_main_frame = !render_frame_host->GetParent();
  int frame_process_id = render_frame_host->GetProcess()->GetID();
  int frame_routing_id = render_frame_host->GetRoutingID();
  Emit("did-fail-load", error_code, error_description, url, is_main_frame,
       frame_process_id, frame_routing_id);
}

void WebContents::DidStartLoading() {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading() {
  Emit("did-stop-loading");
}

bool WebContents::EmitNavigationEvent(
    const std::string& event,
    content::NavigationHandle* navigation_handle) {
  bool is_main_frame = navigation_handle->IsInMainFrame();
  int frame_tree_node_id = navigation_handle->GetFrameTreeNodeId();
  content::FrameTreeNode* frame_tree_node =
      content::FrameTreeNode::GloballyFindByID(frame_tree_node_id);
  content::RenderFrameHostManager* render_manager =
      frame_tree_node->render_manager();
  content::RenderFrameHost* frame_host = nullptr;
  if (render_manager) {
    frame_host = render_manager->speculative_frame_host();
    if (!frame_host)
      frame_host = render_manager->current_frame_host();
  }
  int frame_process_id = -1, frame_routing_id = -1;
  if (frame_host) {
    frame_process_id = frame_host->GetProcess()->GetID();
    frame_routing_id = frame_host->GetRoutingID();
  }
  bool is_same_document = navigation_handle->IsSameDocument();
  auto url = navigation_handle->GetURL();
  return Emit(event, url, is_same_document, is_main_frame, frame_process_id,
              frame_routing_id);
}

void WebContents::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  EmitNavigationEvent("did-start-navigation", navigation_handle);
}

void WebContents::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  EmitNavigationEvent("did-redirect-navigation", navigation_handle);
}

void WebContents::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted())
    return;
  bool is_main_frame = navigation_handle->IsInMainFrame();
  content::RenderFrameHost* frame_host =
      navigation_handle->GetRenderFrameHost();
  int frame_process_id = -1, frame_routing_id = -1;
  if (frame_host) {
    frame_process_id = frame_host->GetProcess()->GetID();
    frame_routing_id = frame_host->GetRoutingID();
  }
  if (!navigation_handle->IsErrorPage()) {
    auto url = navigation_handle->GetURL();
    bool is_same_document = navigation_handle->IsSameDocument();
    if (is_same_document) {
      Emit("did-navigate-in-page", url, is_main_frame, frame_process_id,
           frame_routing_id);
    } else {
      const net::HttpResponseHeaders* http_response =
          navigation_handle->GetResponseHeaders();
      std::string http_status_text;
      int http_response_code = -1;
      if (http_response) {
        http_status_text = http_response->GetStatusText();
        http_response_code = http_response->response_code();
      }
      Emit("did-frame-navigate", url, http_response_code, http_status_text,
           is_main_frame, frame_process_id, frame_routing_id);
      if (is_main_frame) {
        Emit("did-navigate", url, http_response_code, http_status_text);
      }
    }
    if (IsGuest())
      Emit("load-commit", url, is_main_frame);
  } else {
    auto url = navigation_handle->GetURL();
    int code = navigation_handle->GetNetErrorCode();
    auto description = net::ErrorToShortString(code);
    Emit("did-fail-provisional-load", code, description, url, is_main_frame,
         frame_process_id, frame_routing_id);

    // Do not emit "did-fail-load" for canceled requests.
    if (code != net::ERR_ABORTED)
      Emit("did-fail-load", code, description, url, is_main_frame,
           frame_process_id, frame_routing_id);
  }
}

void WebContents::TitleWasSet(content::NavigationEntry* entry) {
  base::string16 final_title;
  bool explicit_set = true;
  if (entry) {
    auto title = entry->GetTitle();
    auto url = entry->GetURL();
    if (url.SchemeIsFile() && title.empty()) {
      final_title = base::UTF8ToUTF16(url.ExtractFileName());
      explicit_set = false;
    } else {
      final_title = title;
    }
  }
  Emit("page-title-updated", final_title, explicit_set);
}

void WebContents::DidUpdateFaviconURL(
    const std::vector<content::FaviconURL>& urls) {
  std::set<GURL> unique_urls;
  for (const auto& iter : urls) {
    if (iter.icon_type != content::FaviconURL::IconType::kFavicon)
      continue;
    const GURL& url = iter.icon_url;
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
  base::Value tab_id(ID());
  managed_web_contents()->CallClientFunction("DevToolsAPI.setInspectedTabId",
                                             &tab_id, nullptr, nullptr);

  // Inherit owner window in devtools when it doesn't have one.
  auto* devtools = managed_web_contents()->GetDevToolsWebContents();
  bool has_window = devtools->GetUserData(NativeWindowRelay::UserDataKey());
  if (owner_window() && !has_window)
    handle->SetOwnerWindow(devtools, owner_window());

  Emit("devtools-opened");
}

void WebContents::DevToolsClosed() {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  devtools_web_contents_.Reset();

  Emit("devtools-closed");
}

#if defined(TOOLKIT_VIEWS) && !defined(OS_MACOSX)
void WebContents::ShowAutofillPopup(content::RenderFrameHost* frame_host,
                                    const gfx::RectF& bounds,
                                    const std::vector<base::string16>& values,
                                    const std::vector<base::string16>& labels) {
  bool offscreen = IsOffScreen() || (embedder_ && embedder_->IsOffScreen());
  gfx::RectF popup_bounds(bounds);
  content::RenderFrameHost* embedder_frame_host = nullptr;
  if (embedder_) {
    auto* embedder_view = embedder_->web_contents()->GetMainFrame()->GetView();
    auto* view = web_contents()->GetMainFrame()->GetView();
    auto offset = view->GetViewBounds().origin() -
                  embedder_view->GetViewBounds().origin();
    popup_bounds.Offset(offset.x(), offset.y());
    embedder_frame_host = embedder_->web_contents()->GetMainFrame();
  }

  CommonWebContentsDelegate::ShowAutofillPopup(
      frame_host, embedder_frame_host, offscreen, popup_bounds, values, labels);
}
#endif

bool WebContents::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebContents, message)
    IPC_MESSAGE_HANDLER_CODE(ViewHostMsg_SetCursor, OnCursorChange,
                             handled = false)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

bool WebContents::OnMessageReceived(const IPC::Message& message,
                                    content::RenderFrameHost* frame_host) {
  bool handled = true;
  FrameDispatchHelper helper = {this, frame_host};
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(WebContents, message, frame_host)
    IPC_MESSAGE_HANDLER(AtomFrameHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_FORWARD_DELAY_REPLY(AtomFrameHostMsg_Message_Sync, &helper,
                                    FrameDispatchHelper::OnRendererMessageSync)
    IPC_MESSAGE_HANDLER(AtomFrameHostMsg_Message_To, OnRendererMessageTo)
    IPC_MESSAGE_FORWARD_DELAY_REPLY(
        AtomFrameHostMsg_SetTemporaryZoomLevel, &helper,
        FrameDispatchHelper::OnSetTemporaryZoomLevel)
    IPC_MESSAGE_FORWARD_DELAY_REPLY(AtomFrameHostMsg_GetZoomLevel, &helper,
                                    FrameDispatchHelper::OnGetZoomLevel)
#if defined(TOOLKIT_VIEWS) && !defined(OS_MACOSX)
    IPC_MESSAGE_HANDLER(AtomAutofillFrameHostMsg_ShowPopup, ShowAutofillPopup)
    IPC_MESSAGE_HANDLER(AtomAutofillFrameHostMsg_HidePopup, HideAutofillPopup)
#endif
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

// There are three ways of destroying a webContents:
// 1. call webContents.destroy();
// 2. garbage collection;
// 3. user closes the window of webContents;
// 4. the embedder detaches the frame.
// For webview both #1 and #4 may happen, for BrowserWindow both #1 and #3 may
// happen. The #2 should never happen for webContents, because webview is
// managed by GuestViewManager, and BrowserWindow's webContents is managed
// by api::BrowserWindow.
// For #1, the destructor will do the cleanup work and we only need to make
// sure "destroyed" event is emitted. For #3, the content::WebContents will
// be destroyed on close, and WebContentsDestroyed would be called for it, so
// we need to make sure the api::WebContents is also deleted.
// For #4, the WebContents will be destroyed by embedder.
void WebContents::WebContentsDestroyed() {
  // Cleanup relationships with other parts.
  RemoveFromWeakMap();

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();

  Emit("destroyed");

  // For guest view based on OOPIF, the WebContents is released by the embedder
  // frame, and we need to clear the reference to the memory.
  if (IsGuest() && managed_web_contents()) {
    managed_web_contents()->ReleaseWebContents();
    ResetManagedWebContents(false);
  }

  // Destroy the native class in next tick.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, GetDestroyClosure());
}

void WebContents::NavigationEntryCommitted(
    const content::LoadCommittedDetails& details) {
  Emit("navigation-entry-commited", details.entry->GetURL(),
       details.is_same_document, details.did_replace_entry);
}

int WebContents::GetProcessID() const {
  return web_contents()->GetMainFrame()->GetProcess()->GetID();
}

base::ProcessId WebContents::GetOSProcessID() const {
  base::ProcessHandle process_handle =
      web_contents()->GetMainFrame()->GetProcess()->GetHandle();
  return base::GetProcId(process_handle);
}

WebContents::Type WebContents::GetType() const {
  return type_;
}

bool WebContents::Equal(const WebContents* web_contents) const {
  return ID() == web_contents->ID();
}

void WebContents::LoadURL(const GURL& url, const mate::Dictionary& options) {
  if (!url.is_valid() || url.spec().size() > url::kMaxURLChars) {
    Emit("did-fail-load", static_cast<int>(net::ERR_INVALID_URL),
         net::ErrorToShortString(net::ERR_INVALID_URL),
         url.possibly_invalid_spec(), true);
    return;
  }

  content::NavigationController::LoadURLParams params(url);

  if (!options.Get("httpReferrer", &params.referrer)) {
    GURL http_referrer;
    if (options.Get("httpReferrer", &http_referrer))
      params.referrer = content::Referrer(http_referrer.GetAsReferrer(),
                                          blink::kWebReferrerPolicyDefault);
  }

  std::string user_agent;
  if (options.Get("userAgent", &user_agent))
    web_contents()->SetUserAgentOverride(user_agent, false);

  std::string extra_headers;
  if (options.Get("extraHeaders", &extra_headers))
    params.extra_headers = extra_headers;

  scoped_refptr<network::ResourceRequestBody> body;
  if (options.Get("postData", &body)) {
    params.post_data = body;
    params.load_type = content::NavigationController::LOAD_TYPE_HTTP_POST;
  }

  GURL base_url_for_data_url;
  if (options.Get("baseURLForDataURL", &base_url_for_data_url)) {
    params.base_url_for_data_url = base_url_for_data_url;
    params.load_type = content::NavigationController::LOAD_TYPE_DATA;
  }

  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  params.should_clear_history_list = true;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  web_contents()->GetController().LoadURLWithParams(params);

  // Set the background color of RenderWidgetHostView.
  // We have to call it right after LoadURL because the RenderViewHost is only
  // created after loading a page.
  auto* const view = web_contents()->GetRenderWidgetHostView();
  if (view) {
    auto* web_preferences = WebContentsPreferences::From(web_contents());
    std::string color_name;
    if (web_preferences->GetPreference(options::kBackgroundColor,
                                       &color_name)) {
      view->SetBackgroundColor(ParseHexColor(color_name));
    } else {
      view->SetBackgroundColor(SK_ColorTRANSPARENT);
    }
  }
}

void WebContents::DownloadURL(const GURL& url) {
  auto* browser_context = web_contents()->GetBrowserContext();
  auto* download_manager =
      content::BrowserContext::GetDownloadManager(browser_context);
  std::unique_ptr<download::DownloadUrlParameters> download_params(
      content::DownloadRequestUtils::CreateDownloadForWebContentsMainFrame(
          web_contents(), url, NO_TRAFFIC_ANNOTATION_YET));
  download_manager->DownloadUrl(std::move(download_params));
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
  return web_contents()->IsLoadingToDifferentDocument();
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

const std::string WebContents::GetWebRTCIPHandlingPolicy() const {
  return web_contents()->GetMutableRendererPrefs()->webrtc_ip_handling_policy;
}

void WebContents::SetWebRTCIPHandlingPolicy(
    const std::string& webrtc_ip_handling_policy) {
  if (GetWebRTCIPHandlingPolicy() == webrtc_ip_handling_policy)
    return;
  web_contents()->GetMutableRendererPrefs()->webrtc_ip_handling_policy =
      webrtc_ip_handling_policy;

  content::RenderViewHost* host = web_contents()->GetRenderViewHost();
  if (host)
    host->SyncRendererPrefs();
}

bool WebContents::IsCrashed() const {
  return web_contents()->IsCrashed();
}

void WebContents::SetUserAgent(const std::string& user_agent,
                               mate::Arguments* args) {
  web_contents()->SetUserAgentOverride(user_agent, false);
}

std::string WebContents::GetUserAgent() {
  return web_contents()->GetUserAgentOverride();
}

bool WebContents::SavePage(const base::FilePath& full_file_path,
                           const content::SavePageType& save_type,
                           const SavePageHandler::SavePageCallback& callback) {
  auto* handler = new SavePageHandler(web_contents(), callback);
  return handler->Handle(full_file_path, save_type);
}

void WebContents::OpenDevTools(mate::Arguments* args) {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  std::string state;
  if (type_ == WEB_VIEW || !owner_window()) {
    state = "detach";
  }
  if (args && args->Length() == 1) {
    mate::Dictionary options;
    if (args->GetNext(&options)) {
      options.Get("mode", &state);
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

  auto* frame_host = web_contents()->GetMainFrame();
  if (frame_host) {
    auto* widget_host =
        frame_host ? frame_host->GetView()->GetRenderWidgetHost() : nullptr;
    if (!widget_host)
      return;
    widget_host->Send(
        new ViewMsg_EnableDeviceEmulation(widget_host->GetRoutingID(), params));
  }
}

void WebContents::DisableDeviceEmulation() {
  if (type_ == REMOTE)
    return;

  auto* frame_host = web_contents()->GetMainFrame();
  if (frame_host) {
    auto* widget_host =
        frame_host ? frame_host->GetView()->GetRenderWidgetHost() : nullptr;
    if (!widget_host)
      return;
    widget_host->Send(
        new ViewMsg_DisableDeviceEmulation(widget_host->GetRoutingID()));
  }
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

  if (!enable_devtools_)
    return;

  if (!managed_web_contents()->GetDevToolsWebContents())
    OpenDevTools(nullptr);
  managed_web_contents()->InspectElement(x, y);
}

void WebContents::InspectServiceWorker() {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeServiceWorker) {
      OpenDevTools(nullptr);
      managed_web_contents()->AttachTo(agent_host);
      break;
    }
  }
}

void WebContents::HasServiceWorker(const base::Callback<void(bool)>& callback) {
  auto* context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  struct WrappedCallback {
    base::Callback<void(bool)> callback_;
    explicit WrappedCallback(const base::Callback<void(bool)>& callback)
        : callback_(callback) {}
    void Run(content::ServiceWorkerCapability capability) {
      callback_.Run(capability !=
                    content::ServiceWorkerCapability::NO_SERVICE_WORKER);
      delete this;
    }
  };

  auto* wrapped_callback = new WrappedCallback(callback);

  context->CheckHasServiceWorker(
      web_contents()->GetLastCommittedURL(), GURL::EmptyGURL(),
      base::BindOnce(&WrappedCallback::Run,
                     base::Unretained(wrapped_callback)));
}

void WebContents::UnregisterServiceWorker(
    const base::Callback<void(bool)>& callback) {
  auto* context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->UnregisterServiceWorker(web_contents()->GetLastCommittedURL(),
                                   callback);
}

void WebContents::SetIgnoreMenuShortcuts(bool ignore) {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  DCHECK(web_preferences);
  web_preferences->preference()->SetKey("ignoreMenuShortcuts",
                                        base::Value(ignore));
}

void WebContents::SetAudioMuted(bool muted) {
  web_contents()->SetAudioMuted(muted);
}

bool WebContents::IsAudioMuted() {
  return web_contents()->IsAudioMuted();
}

bool WebContents::IsCurrentlyAudible() {
  return web_contents()->IsCurrentlyAudible();
}

void WebContents::Print(mate::Arguments* args) {
  PrintSettings settings = {false, false, base::string16()};
  if (args->Length() >= 1 && !args->GetNext(&settings)) {
    args->ThrowError();
    return;
  }
  auto* print_view_manager_basic_ptr =
      printing::PrintViewManagerBasic::FromWebContents(web_contents());
  if (args->Length() == 2) {
    base::Callback<void(bool)> callback;
    if (!args->GetNext(&callback)) {
      args->ThrowError();
      return;
    }
    print_view_manager_basic_ptr->SetCallback(callback);
  }
  print_view_manager_basic_ptr->PrintNow(
      web_contents()->GetMainFrame(), settings.silent,
      settings.print_background, settings.device_name);
}

std::vector<printing::PrinterBasicInfo> WebContents::GetPrinterList() {
  std::vector<printing::PrinterBasicInfo> printers;
  auto print_backend = printing::PrintBackend::CreateInstance(nullptr);
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  print_backend->EnumeratePrinters(&printers);
  return printers;
}

void WebContents::PrintToPDF(const base::DictionaryValue& setting,
                             const PrintToPDFCallback& callback) {
  printing::PrintPreviewMessageHandler::FromWebContents(web_contents())
      ->PrintToPDF(setting, callback);
}

void WebContents::AddWorkSpace(mate::Arguments* args,
                               const base::FilePath& path) {
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
  DevToolsAddFileSystem(std::string(), path);
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
  web_contents()->CollapseSelection();
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
  auto* const view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->ShowDefinitionForSelection();
#endif
}

void WebContents::CopyImageAt(int x, int y) {
  auto* const host = web_contents()->GetMainFrame();
  if (host)
    host->CopyImageAt(x, y);
}

void WebContents::Focus() {
  web_contents()->Focus();
}

#if !defined(OS_MACOSX)
bool WebContents::IsFocused() const {
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return false;

  if (GetType() != BACKGROUND_PAGE) {
    auto* window = web_contents()->GetNativeView()->GetToplevelWindow();
    if (window && !window->IsVisible())
      return false;
  }

  return view->HasFocus();
}
#endif

void WebContents::TabTraverse(bool reverse) {
  web_contents()->FocusThroughTabTraversal(reverse);
}

bool WebContents::SendIPCMessage(bool all_frames,
                                 const std::string& channel,
                                 const base::ListValue& args) {
  return SendIPCMessageWithSender(all_frames, channel, args);
}

bool WebContents::SendIPCMessageWithSender(bool all_frames,
                                           const std::string& channel,
                                           const base::ListValue& args,
                                           int32_t sender_id) {
  auto* frame_host = web_contents()->GetMainFrame();
  if (frame_host) {
    return frame_host->Send(new AtomFrameMsg_Message(
        frame_host->GetRoutingID(), all_frames, channel, args, sender_id));
  }
  return false;
}

void WebContents::SendInputEvent(v8::Isolate* isolate,
                                 v8::Local<v8::Value> input_event) {
  content::RenderWidgetHostView* view =
      web_contents()->GetRenderWidgetHostView();
  if (!view)
    return;

  content::RenderWidgetHost* rwh = view->GetRenderWidgetHost();
  blink::WebInputEvent::Type type =
      mate::GetWebInputEventType(isolate, input_event);
  if (blink::WebInputEvent::IsMouseEventType(type)) {
    blink::WebMouseEvent mouse_event;
    if (mate::ConvertFromV8(isolate, input_event, &mouse_event)) {
      if (IsOffScreen()) {
#if defined(ENABLE_OSR)
        GetOffScreenRenderWidgetHostView()->SendMouseEvent(mouse_event);
#endif
      } else {
        rwh->ForwardMouseEvent(mouse_event);
      }
      return;
    }
  } else if (blink::WebInputEvent::IsKeyboardEventType(type)) {
    content::NativeWebKeyboardEvent keyboard_event(
        blink::WebKeyboardEvent::kRawKeyDown,
        blink::WebInputEvent::kNoModifiers, ui::EventTimeForNow());
    if (mate::ConvertFromV8(isolate, input_event, &keyboard_event)) {
      rwh->ForwardKeyboardEvent(keyboard_event);
      return;
    }
  } else if (type == blink::WebInputEvent::kMouseWheel) {
    blink::WebMouseWheelEvent mouse_wheel_event;
    if (mate::ConvertFromV8(isolate, input_event, &mouse_wheel_event)) {
      if (IsOffScreen()) {
#if defined(ENABLE_OSR)
        GetOffScreenRenderWidgetHostView()->SendMouseWheelEvent(
            mouse_wheel_event);
#endif
      } else {
        rwh->ForwardWheelEvent(mouse_wheel_event);
      }
      return;
    }
  }

  isolate->ThrowException(
      v8::Exception::Error(mate::StringToV8(isolate, "Invalid event object")));
}

void WebContents::BeginFrameSubscription(mate::Arguments* args) {
  bool only_dirty = false;
  FrameSubscriber::FrameCaptureCallback callback;

  args->GetNext(&only_dirty);
  if (!args->GetNext(&callback)) {
    args->ThrowError();
    return;
  }

  frame_subscriber_.reset(
      new FrameSubscriber(isolate(), web_contents(), callback, only_dirty));
}

void WebContents::EndFrameSubscription() {
  frame_subscriber_.reset();
}

void WebContents::StartDrag(const mate::Dictionary& item,
                            mate::Arguments* args) {
  base::FilePath file;
  std::vector<base::FilePath> files;
  if (!item.Get("files", &files) && item.Get("file", &file)) {
    files.push_back(file);
  }

  mate::Handle<NativeImage> icon;
  if (!item.Get("icon", &icon) && !file.empty()) {
    // TODO(zcbenz): Set default icon from file.
  }

  // Error checking.
  if (icon.IsEmpty()) {
    args->ThrowError("Must specify 'icon' option");
    return;
  }

#if defined(OS_MACOSX)
  // NSWindow.dragImage requires a non-empty NSImage
  if (icon->image().IsEmpty()) {
    args->ThrowError("Must specify non-empty 'icon' option");
    return;
  }
#endif

  // Start dragging.
  if (!files.empty()) {
    base::MessageLoop::ScopedNestableTaskAllower allow(
        base::MessageLoop::current());
    DragFileItems(files, icon->image(), web_contents()->GetNativeView());
  } else {
    args->ThrowError("Must specify either 'file' or 'files' option");
  }
}

void WebContents::CapturePage(mate::Arguments* args) {
  gfx::Rect rect;
  base::Callback<void(const gfx::Image&)> callback;

  if (!(args->Length() == 1 && args->GetNext(&callback)) &&
      !(args->Length() == 2 && args->GetNext(&rect) &&
        args->GetNext(&callback))) {
    args->ThrowError();
    return;
  }

  auto* const view = web_contents()->GetRenderWidgetHostView();
  if (!view) {
    callback.Run(gfx::Image());
    return;
  }

  // Capture full page if user doesn't specify a |rect|.
  const gfx::Size view_size =
      rect.IsEmpty() ? view->GetViewBounds().size() : rect.size();

  // By default, the requested bitmap size is the view size in screen
  // coordinates.  However, if there's more pixel detail available on the
  // current system, increase the requested bitmap size to capture it all.
  gfx::Size bitmap_size = view_size;
  const gfx::NativeView native_view = view->GetNativeView();
  const float scale = display::Screen::GetScreen()
                          ->GetDisplayNearestView(native_view)
                          .device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ScaleToCeiledSize(view_size, scale);

  view->CopyFromSurface(gfx::Rect(rect.origin(), view_size), bitmap_size,
                        base::BindOnce(&OnCapturePageDone, callback));
}

void WebContents::OnCursorChange(const content::WebCursor& cursor) {
  content::CursorInfo info;
  cursor.GetCursorInfo(&info);

  if (cursor.IsCustom()) {
    Emit("cursor-changed", CursorTypeToString(info),
         gfx::Image::CreateFrom1xBitmap(info.custom_image),
         info.image_scale_factor,
         gfx::Size(info.custom_image.width(), info.custom_image.height()),
         info.hotspot);
  } else {
    Emit("cursor-changed", CursorTypeToString(info));
  }
}

bool WebContents::IsGuest() const {
  return type_ == WEB_VIEW;
}

void WebContents::AttachToIframe(content::WebContents* embedder_web_contents,
                                 int embedder_frame_id) {
  if (guest_delegate_)
    guest_delegate_->AttachToIframe(embedder_web_contents, embedder_frame_id);
}

bool WebContents::IsOffScreen() const {
#if defined(ENABLE_OSR)
  return type_ == OFF_SCREEN;
#else
  return false;
#endif
}

void WebContents::OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap) {
  Emit("paint", dirty_rect, gfx::Image::CreateFrom1xBitmap(bitmap));
}

void WebContents::StartPainting() {
  if (!IsOffScreen())
    return;

#if defined(ENABLE_OSR)
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetPainting(true);
#endif
}

void WebContents::StopPainting() {
  if (!IsOffScreen())
    return;

#if defined(ENABLE_OSR)
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetPainting(false);
#endif
}

bool WebContents::IsPainting() const {
  if (!IsOffScreen())
    return false;

#if defined(ENABLE_OSR)
  auto* osr_wcv = GetOffScreenWebContentsView();
  return osr_wcv && osr_wcv->IsPainting();
#else
  return false;
#endif
}

void WebContents::SetFrameRate(int frame_rate) {
  if (!IsOffScreen())
    return;

#if defined(ENABLE_OSR)
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetFrameRate(frame_rate);
#endif
}

int WebContents::GetFrameRate() const {
  if (!IsOffScreen())
    return 0;

#if defined(ENABLE_OSR)
  auto* osr_wcv = GetOffScreenWebContentsView();
  return osr_wcv ? osr_wcv->GetFrameRate() : 0;
#else
  return 0;
#endif
}

void WebContents::Invalidate() {
  if (IsOffScreen()) {
#if defined(ENABLE_OSR)
    auto* osr_rwhv = GetOffScreenRenderWidgetHostView();
    if (osr_rwhv)
      osr_rwhv->Invalidate();
#endif
  } else {
    auto* const window = owner_window();
    if (window)
      window->Invalidate();
  }
}

gfx::Size WebContents::GetSizeForNewRenderView(content::WebContents* wc) const {
  if (IsOffScreen() && wc == web_contents()) {
    auto* relay = NativeWindowRelay::FromWebContents(web_contents());
    if (relay) {
      return relay->window->GetSize();
    }
  }

  return gfx::Size();
}

void WebContents::SetZoomLevel(double level) {
  zoom_controller_->SetZoomLevel(level);
}

double WebContents::GetZoomLevel() const {
  return zoom_controller_->GetZoomLevel();
}

void WebContents::SetZoomFactor(double factor) {
  auto level = content::ZoomFactorToZoomLevel(factor);
  SetZoomLevel(level);
}

double WebContents::GetZoomFactor() const {
  auto level = GetZoomLevel();
  return content::ZoomLevelToZoomFactor(level);
}

void WebContents::OnSetTemporaryZoomLevel(content::RenderFrameHost* rfh,
                                          double level,
                                          IPC::Message* reply_msg) {
  zoom_controller_->SetTemporaryZoomLevel(level);
  double new_level = zoom_controller_->GetZoomLevel();
  AtomFrameHostMsg_SetTemporaryZoomLevel::WriteReplyParams(reply_msg,
                                                           new_level);
  rfh->Send(reply_msg);
}

void WebContents::OnGetZoomLevel(content::RenderFrameHost* rfh,
                                 IPC::Message* reply_msg) {
  AtomFrameHostMsg_GetZoomLevel::WriteReplyParams(reply_msg, GetZoomLevel());
  rfh->Send(reply_msg);
}

v8::Local<v8::Value> WebContents::GetPreloadPath(v8::Isolate* isolate) const {
  if (auto* web_preferences = WebContentsPreferences::From(web_contents())) {
    base::FilePath::StringType preload;
    if (web_preferences->GetPreloadPath(&preload)) {
      return mate::ConvertToV8(isolate, preload);
    }
  }
  return v8::Null(isolate);
}

v8::Local<v8::Value> WebContents::GetWebPreferences(
    v8::Isolate* isolate) const {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  if (!web_preferences)
    return v8::Null(isolate);
  return mate::ConvertToV8(isolate, *web_preferences->preference());
}

v8::Local<v8::Value> WebContents::GetLastWebPreferences(
    v8::Isolate* isolate) const {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  if (!web_preferences)
    return v8::Null(isolate);
  return mate::ConvertToV8(isolate, *web_preferences->last_preference());
}

v8::Local<v8::Value> WebContents::GetOwnerBrowserWindow() const {
  if (owner_window())
    return BrowserWindow::From(isolate(), owner_window());
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

void WebContents::SetEmbedder(const WebContents* embedder) {
  if (embedder) {
    NativeWindow* owner_window = nullptr;
    auto* relay = NativeWindowRelay::FromWebContents(embedder->web_contents());
    if (relay) {
      owner_window = relay->window.get();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);

    content::RenderWidgetHostView* rwhv =
        web_contents()->GetRenderWidgetHostView();
    if (rwhv) {
      rwhv->Hide();
      rwhv->Show();
    }
  }
}

void WebContents::SetDevToolsWebContents(const WebContents* devtools) {
  if (managed_web_contents())
    managed_web_contents()->SetDevToolsWebContents(devtools->web_contents());
}

v8::Local<v8::Value> WebContents::GetNativeView() const {
  gfx::NativeView ptr = web_contents()->GetNativeView();
  auto buffer = node::Buffer::Copy(isolate(), reinterpret_cast<char*>(&ptr),
                                   sizeof(gfx::NativeView));
  if (buffer.IsEmpty())
    return v8::Null(isolate());
  else
    return buffer.ToLocalChecked();
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

void WebContents::GrantOriginAccess(const GURL& url) {
  content::ChildProcessSecurityPolicy::GetInstance()->GrantOrigin(
      web_contents()->GetMainFrame()->GetProcess()->GetID(),
      url::Origin::Create(url));
}

bool WebContents::TakeHeapSnapshot(const base::FilePath& file_path,
                                   const std::string& channel) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  base::File file(file_path,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid())
    return false;

  auto* frame_host = web_contents()->GetMainFrame();
  if (!frame_host)
    return false;

  return frame_host->Send(new AtomFrameMsg_TakeHeapSnapshot(
      frame_host->GetRoutingID(),
      IPC::TakePlatformFileForTransit(std::move(file)), channel));
}

// static
void WebContents::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebContents"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("getProcessId", &WebContents::GetProcessID)
      .SetMethod("getOSProcessId", &WebContents::GetOSProcessID)
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
      .SetMethod("savePage", &WebContents::SavePage)
      .SetMethod("openDevTools", &WebContents::OpenDevTools)
      .SetMethod("closeDevTools", &WebContents::CloseDevTools)
      .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
      .SetMethod("isDevToolsFocused", &WebContents::IsDevToolsFocused)
      .SetMethod("enableDeviceEmulation", &WebContents::EnableDeviceEmulation)
      .SetMethod("disableDeviceEmulation", &WebContents::DisableDeviceEmulation)
      .SetMethod("toggleDevTools", &WebContents::ToggleDevTools)
      .SetMethod("inspectElement", &WebContents::InspectElement)
      .SetMethod("setIgnoreMenuShortcuts", &WebContents::SetIgnoreMenuShortcuts)
      .SetMethod("setAudioMuted", &WebContents::SetAudioMuted)
      .SetMethod("isAudioMuted", &WebContents::IsAudioMuted)
      .SetMethod("isCurrentlyAudible", &WebContents::IsCurrentlyAudible)
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
      .SetMethod("isFocused", &WebContents::IsFocused)
      .SetMethod("tabTraverse", &WebContents::TabTraverse)
      .SetMethod("_send", &WebContents::SendIPCMessage)
      .SetMethod("sendInputEvent", &WebContents::SendInputEvent)
      .SetMethod("beginFrameSubscription", &WebContents::BeginFrameSubscription)
      .SetMethod("endFrameSubscription", &WebContents::EndFrameSubscription)
      .SetMethod("startDrag", &WebContents::StartDrag)
      .SetMethod("isGuest", &WebContents::IsGuest)
      .SetMethod("attachToIframe", &WebContents::AttachToIframe)
      .SetMethod("isOffscreen", &WebContents::IsOffScreen)
      .SetMethod("startPainting", &WebContents::StartPainting)
      .SetMethod("stopPainting", &WebContents::StopPainting)
      .SetMethod("isPainting", &WebContents::IsPainting)
      .SetMethod("setFrameRate", &WebContents::SetFrameRate)
      .SetMethod("getFrameRate", &WebContents::GetFrameRate)
      .SetMethod("invalidate", &WebContents::Invalidate)
      .SetMethod("setZoomLevel", &WebContents::SetZoomLevel)
      .SetMethod("_getZoomLevel", &WebContents::GetZoomLevel)
      .SetMethod("setZoomFactor", &WebContents::SetZoomFactor)
      .SetMethod("_getZoomFactor", &WebContents::GetZoomFactor)
      .SetMethod("getType", &WebContents::GetType)
      .SetMethod("_getPreloadPath", &WebContents::GetPreloadPath)
      .SetMethod("getWebPreferences", &WebContents::GetWebPreferences)
      .SetMethod("getLastWebPreferences", &WebContents::GetLastWebPreferences)
      .SetMethod("getOwnerBrowserWindow", &WebContents::GetOwnerBrowserWindow)
      .SetMethod("hasServiceWorker", &WebContents::HasServiceWorker)
      .SetMethod("unregisterServiceWorker",
                 &WebContents::UnregisterServiceWorker)
      .SetMethod("inspectServiceWorker", &WebContents::InspectServiceWorker)
      .SetMethod("print", &WebContents::Print)
      .SetMethod("getPrinters", &WebContents::GetPrinterList)
      .SetMethod("_printToPDF", &WebContents::PrintToPDF)
      .SetMethod("addWorkSpace", &WebContents::AddWorkSpace)
      .SetMethod("removeWorkSpace", &WebContents::RemoveWorkSpace)
      .SetMethod("showDefinitionForSelection",
                 &WebContents::ShowDefinitionForSelection)
      .SetMethod("copyImageAt", &WebContents::CopyImageAt)
      .SetMethod("capturePage", &WebContents::CapturePage)
      .SetMethod("setEmbedder", &WebContents::SetEmbedder)
      .SetMethod("setDevToolsWebContents", &WebContents::SetDevToolsWebContents)
      .SetMethod("getNativeView", &WebContents::GetNativeView)
      .SetMethod("setWebRTCIPHandlingPolicy",
                 &WebContents::SetWebRTCIPHandlingPolicy)
      .SetMethod("getWebRTCIPHandlingPolicy",
                 &WebContents::GetWebRTCIPHandlingPolicy)
      .SetMethod("_grantOriginAccess", &WebContents::GrantOriginAccess)
      .SetMethod("_takeHeapSnapshot", &WebContents::TakeHeapSnapshot)
      .SetProperty("id", &WebContents::ID)
      .SetProperty("session", &WebContents::Session)
      .SetProperty("hostWebContents", &WebContents::HostWebContents)
      .SetProperty("devToolsWebContents", &WebContents::DevToolsWebContents)
      .SetProperty("debugger", &WebContents::Debugger);
}

AtomBrowserContext* WebContents::GetBrowserContext() const {
  return static_cast<AtomBrowserContext*>(web_contents()->GetBrowserContext());
}

void WebContents::OnRendererMessage(content::RenderFrameHost* frame_host,
                                    const std::string& channel,
                                    const base::ListValue& args) {
  // webContents.emit(channel, new Event(), args...);
  Emit(channel, args);
}

void WebContents::OnRendererMessageSync(content::RenderFrameHost* frame_host,
                                        const std::string& channel,
                                        const base::ListValue& args,
                                        IPC::Message* message) {
  // webContents.emit(channel, new Event(sender, message), args...);
  EmitWithSender(channel, frame_host, message, args);
}

void WebContents::OnRendererMessageTo(content::RenderFrameHost* frame_host,
                                      bool send_to_all,
                                      int32_t web_contents_id,
                                      const std::string& channel,
                                      const base::ListValue& args) {
  auto* web_contents = mate::TrackableObject<WebContents>::FromWeakMapID(
      isolate(), web_contents_id);

  if (web_contents) {
    web_contents->SendIPCMessageWithSender(send_to_all, channel, args, ID());
  }
}

// static
mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate,
    content::WebContents* web_contents) {
  // We have an existing WebContents object in JS.
  auto* existing = TrackableObject::FromWrappedClass(isolate, web_contents);
  if (existing)
    return mate::CreateHandle(isolate, static_cast<WebContents*>(existing));

  // Otherwise create a new WebContents wrapper object.
  return mate::CreateHandle(isolate,
                            new WebContents(isolate, web_contents, REMOTE));
}

mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate,
    content::WebContents* web_contents,
    Type type) {
  // Otherwise create a new WebContents wrapper object.
  return mate::CreateHandle(isolate,
                            new WebContents(isolate, web_contents, type));
}

// static
mate::Handle<WebContents> WebContents::Create(v8::Isolate* isolate,
                                              const mate::Dictionary& options) {
  return mate::CreateHandle(isolate, new WebContents(isolate, options));
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::WebContents;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("WebContents", WebContents::GetConstructor(isolate)->GetFunction());
  dict.SetMethod("create", &WebContents::Create);
  dict.SetMethod("fromId", &mate::TrackableObject<WebContents>::FromWeakMapID);
  dict.SetMethod("getAllWebContents",
                 &mate::TrackableObject<WebContents>::GetAll);
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_web_contents, Initialize)
