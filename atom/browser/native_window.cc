// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window.h"

#include <string>
#include <utility>
#include <vector>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_javascript_dialog_manager.h"
#include "atom/browser/browser.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/window_list.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/json/json_writer.h"
#include "base/prefs/pref_service.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/user_agent.h"
#include "ipc/ipc_message_macros.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include "vendor/brightray/browser/inspectable_web_contents.h"
#include "vendor/brightray/browser/inspectable_web_contents_view.h"
#include "webkit/common/webpreferences.h"

using content::NavigationEntry;

namespace atom {

NativeWindow::NativeWindow(content::WebContents* web_contents,
                           const mate::Dictionary& options)
    : content::WebContentsObserver(web_contents),
      has_frame_(true),
      is_closed_(false),
      node_integration_("except-iframe"),
      has_dialog_attached_(false),
      zoom_factor_(1.0),
      weak_factory_(this),
      inspectable_web_contents_(
          brightray::InspectableWebContents::Create(web_contents)) {
  options.Get(switches::kFrame, &has_frame_);

  // Read icon before window is created.
  gfx::ImageSkia icon;
  if (options.Get(switches::kIcon, &icon))
    icon_.reset(new gfx::Image(icon));

  // Read iframe security before any navigation.
  options.Get(switches::kNodeIntegration, &node_integration_);

  // Read the web preferences.
  scoped_ptr<mate::Dictionary> web_preferences(new mate::Dictionary);
  if (options.Get(switches::kWebPreferences, web_preferences.get()))
    web_preferences_.reset(web_preferences.release());

  // Read the zoom factor before any navigation.
  options.Get(switches::kZoomFactor, &zoom_factor_);

  web_contents->SetDelegate(this);
  inspectable_web_contents()->SetDelegate(this);

  WindowList::AddWindow(this);

  // Override the user agent to contain application and atom-shell's version.
  Browser* browser = Browser::Get();
  std::string product_name = base::StringPrintf(
      "%s/%s Chrome/%s Atom-Shell/" ATOM_VERSION_STRING,
      browser->GetName().c_str(),
      browser->GetVersion().c_str(),
      CHROME_VERSION_STRING);
  web_contents->GetMutableRendererPrefs()->user_agent_override =
      content::BuildUserAgentFromProduct(product_name);

  // Get notified of title updated message.
  registrar_.Add(this, content::NOTIFICATION_WEB_CONTENTS_TITLE_UPDATED,
      content::Source<content::WebContents>(web_contents));
}

NativeWindow::~NativeWindow() {
  // Make sure we have the OnRenderViewDeleted message sent even when the window
  // is destroyed directly.
  DestroyWebContents();

  // It's possible that the windows gets destroyed before it's closed, in that
  // case we need to ensure the OnWindowClosed message is still notified.
  NotifyWindowClosed();
}

// static
NativeWindow* NativeWindow::Create(const mate::Dictionary& options) {
  content::WebContents::CreateParams create_params(AtomBrowserContext::Get());
  return Create(content::WebContents::Create(create_params), options);
}

// static
NativeWindow* NativeWindow::FromRenderView(int process_id, int routing_id) {
  // Stupid iterating.
  WindowList& window_list = *WindowList::GetInstance();
  for (auto w = window_list.begin(); w != window_list.end(); ++w) {
    auto& window = *w;
    content::WebContents* web_contents = window->GetWebContents();
    int window_process_id = web_contents->GetRenderProcessHost()->GetID();
    int window_routing_id = web_contents->GetRoutingID();
    if (window_routing_id == routing_id && window_process_id == process_id)
      return window;
  }

  return NULL;
}

void NativeWindow::InitFromOptions(const mate::Dictionary& options) {
  // Setup window from options.
  int x = -1, y = -1;
  bool center;
  if (options.Get(switches::kX, &x) && options.Get(switches::kY, &y)) {
    int width = -1, height = -1;
    options.Get(switches::kWidth, &width);
    options.Get(switches::kHeight, &height);
    Move(gfx::Rect(x, y, width, height));
  } else if (options.Get(switches::kCenter, &center) && center) {
    Center();
  }
  int min_height = -1, min_width = -1;
  if (options.Get(switches::kMinHeight, &min_height) &&
      options.Get(switches::kMinWidth, &min_width)) {
    SetMinimumSize(gfx::Size(min_width, min_height));
  }
  int max_height = -1, max_width = -1;
  if (options.Get(switches::kMaxHeight, &max_height) &&
      options.Get(switches::kMaxWidth, &max_width)) {
    SetMaximumSize(gfx::Size(max_width, max_height));
  }
  bool resizable;
  if (options.Get(switches::kResizable, &resizable)) {
    SetResizable(resizable);
  }
  bool top;
  if (options.Get(switches::kAlwaysOnTop, &top) && top) {
    SetAlwaysOnTop(true);
  }
  bool fullscreen;
  if (options.Get(switches::kFullscreen, &fullscreen) && fullscreen) {
    SetFullscreen(true);
  }
  bool skip;
  if (options.Get(switches::kSkipTaskbar, &skip) && skip) {
    SetSkipTaskbar(skip);
  }
  bool kiosk;
  if (options.Get(switches::kKiosk, &kiosk) && kiosk) {
    SetKiosk(kiosk);
  }
  std::string title("Atom Shell");
  options.Get(switches::kTitle, &title);
  SetTitle(title);

  // Then show it.
  bool show = true;
  options.Get(switches::kShow, &show);
  if (show)
    Show();
}

void NativeWindow::SetRepresentedFilename(const std::string& filename) {
}

void NativeWindow::SetDocumentEdited(bool edited) {
}

void NativeWindow::SetMenu(ui::MenuModel* menu) {
}

bool NativeWindow::HasModalDialog() {
  return has_dialog_attached_;
}

void NativeWindow::OpenDevTools() {
  inspectable_web_contents()->ShowDevTools();
}

void NativeWindow::CloseDevTools() {
  inspectable_web_contents()->CloseDevTools();
}

bool NativeWindow::IsDevToolsOpened() {
  return inspectable_web_contents()->IsDevToolsViewShowing();
}

void NativeWindow::InspectElement(int x, int y) {
  OpenDevTools();
  content::RenderViewHost* rvh = GetWebContents()->GetRenderViewHost();
  scoped_refptr<content::DevToolsAgentHost> agent(
      content::DevToolsAgentHost::GetOrCreateFor(rvh));
  agent->InspectElement(x, y);
}

void NativeWindow::FocusOnWebView() {
  GetWebContents()->GetRenderViewHost()->Focus();
}

void NativeWindow::BlurWebView() {
  GetWebContents()->GetRenderViewHost()->Blur();
}

bool NativeWindow::IsWebViewFocused() {
  content::RenderWidgetHostView* host_view =
      GetWebContents()->GetRenderViewHost()->GetView();
  return host_view && host_view->HasFocus();
}

void NativeWindow::CapturePage(const gfx::Rect& rect,
                               const CapturePageCallback& callback) {
  content::RenderViewHost* render_view_host =
      GetWebContents()->GetRenderViewHost();
  content::RenderWidgetHostView* render_widget_host_view =
      render_view_host->GetView();

  if (!render_widget_host_view) {
    callback.Run(std::vector<unsigned char>());
    return;
  }

  gfx::Rect flipped_y_rect = rect;
  flipped_y_rect.set_y(-rect.y());

  gfx::Size size;
  if (flipped_y_rect.IsEmpty())
    size = render_widget_host_view->GetViewBounds().size();
  else
    size = flipped_y_rect.size();

  GetWebContents()->GetRenderViewHost()->CopyFromBackingStore(
      flipped_y_rect,
      size,
      base::Bind(&NativeWindow::OnCapturePageDone,
                 weak_factory_.GetWeakPtr(),
                 callback),
      SkBitmap::kARGB_8888_Config);
}

void NativeWindow::DestroyWebContents() {
  if (!inspectable_web_contents_)
    return;

  inspectable_web_contents_.reset();
}

void NativeWindow::CloseWebContents() {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(NativeWindowObserver,
                    observers_,
                    WillCloseWindow(&prevent_default));
  if (prevent_default) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

  content::WebContents* web_contents(GetWebContents());
  if (!web_contents) {
    CloseImmediately();
    return;
  }

  // Assume the window is not responding if it doesn't cancel the close and is
  // not closed in 5s, in this way we can quickly show the unresponsive
  // dialog when the window is busy executing some script withouth waiting for
  // the unresponsive timeout.
  if (window_unresposive_closure_.IsCancelled())
    ScheduleUnresponsiveEvent(5000);

  if (web_contents->NeedToFireBeforeUnload())
    web_contents->GetMainFrame()->DispatchBeforeUnload(false);
  else
    web_contents->Close();
}

content::WebContents* NativeWindow::GetWebContents() const {
  if (!inspectable_web_contents_)
    return NULL;
  return inspectable_web_contents()->GetWebContents();
}

content::WebContents* NativeWindow::GetDevToolsWebContents() const {
  if (!inspectable_web_contents_)
    return NULL;
  return inspectable_web_contents()->devtools_web_contents();
}

void NativeWindow::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line, int child_process_id) {
  // Append --node-integration to renderer process.
  command_line->AppendSwitchASCII(switches::kNodeIntegration,
                                  node_integration_);

  // Append --zoom-factor.
  if (zoom_factor_ != 1.0)
    command_line->AppendSwitchASCII(switches::kZoomFactor,
                                    base::DoubleToString(zoom_factor_));
}

void NativeWindow::OverrideWebkitPrefs(const GURL& url, WebPreferences* prefs) {
  // FIXME Disable accelerated composition in frameless window.
  if (!has_frame_)
    prefs->accelerated_compositing_enabled = false;

  bool b;
  std::vector<base::FilePath> list;
  if (!web_preferences_)
    return;
  if (web_preferences_->Get("javascript", &b))
    prefs->javascript_enabled = b;
  if (web_preferences_->Get("web-security", &b))
    prefs->web_security_enabled = b;
  if (web_preferences_->Get("images", &b))
    prefs->images_enabled = b;
  if (web_preferences_->Get("java", &b))
    prefs->java_enabled = b;
  if (web_preferences_->Get("text-areas-are-resizable", &b))
    prefs->text_areas_are_resizable = b;
  if (web_preferences_->Get("webgl", &b))
    prefs->experimental_webgl_enabled = b;
  if (web_preferences_->Get("webaudio", &b))
    prefs->webaudio_enabled = b;
  if (web_preferences_->Get("accelerated-compositing", &b))
    prefs->accelerated_compositing_enabled = b;
  if (web_preferences_->Get("plugins", &b))
    prefs->plugins_enabled = b;
  if (web_preferences_->Get("extra-plugin-dirs", &list))
    for (size_t i = 0; i < list.size(); ++i)
      content::PluginService::GetInstance()->AddExtraPluginDir(list[i]);
}

void NativeWindow::NotifyWindowClosed() {
  if (is_closed_)
    return;

  is_closed_ = true;
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowClosed());

  // Do not receive any notification after window has been closed, there is a
  // crash that seems to be caused by this: http://git.io/YqMG5g.
  registrar_.RemoveAll();

  WindowList::RemoveWindow(this);
}

void NativeWindow::NotifyWindowBlur() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowBlur());
}

void NativeWindow::NotifyWindowFocus() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowFocus());
}

// In atom-shell all reloads and navigations started by renderer process would
// be redirected to this method, so we can have precise control of how we
// would open the url (in our case, is to restart the renderer process). See
// AtomRendererClient::ShouldFork for how this is done.
content::WebContents* NativeWindow::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (params.disposition != CURRENT_TAB)
      return NULL;

  content::NavigationController::LoadURLParams load_url_params(params.url);
  load_url_params.referrer = params.referrer;
  load_url_params.transition_type = params.transition;
  load_url_params.extra_headers = params.extra_headers;
  load_url_params.should_replace_current_entry =
      params.should_replace_current_entry;
  load_url_params.is_renderer_initiated = params.is_renderer_initiated;
  load_url_params.transferred_global_request_id =
      params.transferred_global_request_id;

  source->GetController().LoadURLWithParams(load_url_params);
  return source;
}

content::JavaScriptDialogManager* NativeWindow::GetJavaScriptDialogManager() {
  if (!dialog_manager_)
    dialog_manager_.reset(new AtomJavaScriptDialogManager);

  return dialog_manager_.get();
}

void NativeWindow::BeforeUnloadFired(content::WebContents* tab,
                                     bool proceed,
                                     bool* proceed_to_fire_unload) {
  *proceed_to_fire_unload = proceed;

  if (!proceed) {
    WindowList::WindowCloseCancelled(this);

    // Cancel unresponsive event when window close is cancelled.
    window_unresposive_closure_.Cancel();
  }
}

void NativeWindow::RequestToLockMouse(content::WebContents* web_contents,
                                      bool user_gesture,
                                      bool last_unlocked_by_target) {
  GetWebContents()->GotResponseToLockMouseRequest(true);
}

bool NativeWindow::CanOverscrollContent() const {
  return false;
}

void NativeWindow::ActivateContents(content::WebContents* contents) {
  FocusOnWebView();
}

void NativeWindow::DeactivateContents(content::WebContents* contents) {
  BlurWebView();
}

void NativeWindow::MoveContents(content::WebContents* source,
                                const gfx::Rect& pos) {
  SetPosition(pos.origin());
  SetSize(pos.size());
}

void NativeWindow::CloseContents(content::WebContents* source) {
  // Destroy the WebContents before we close the window.
  DestroyWebContents();

  // When the web contents is gone, close the window immediately, but the
  // memory will not be freed until you call delete.
  // In this way, it would be safe to manage windows via smart pointers. If you
  // want to free memory when the window is closed, you can do deleting by
  // overriding the OnWindowClosed method in the observer.
  CloseImmediately();

  // Do not sent "unresponsive" event after window is closed.
  window_unresposive_closure_.Cancel();
}

bool NativeWindow::IsPopupOrPanel(const content::WebContents* source) const {
  // Only popup window can use things like window.moveTo.
  return true;
}

void NativeWindow::RendererUnresponsive(content::WebContents* source) {
  // Schedule the unresponsive shortly later, since we may receive the
  // responsive event soon. This could happen after the whole application had
  // blocked for a while.
  // Also notice that when closing this event would be ignored because we have
  // explicity started a close timeout counter. This is on purpose because we
  // don't want the unresponsive event to be sent too early when user is closing
  // the window.
  ScheduleUnresponsiveEvent(50);
}

void NativeWindow::RendererResponsive(content::WebContents* source) {
  window_unresposive_closure_.Cancel();
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnRendererResponsive());
}

void NativeWindow::BeforeUnloadFired(const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
}

bool NativeWindow::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NativeWindow, message)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_UpdateDraggableRegions,
                        UpdateDraggableRegions)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void NativeWindow::Observe(int type,
                           const content::NotificationSource& source,
                           const content::NotificationDetails& details) {
  if (type == content::NOTIFICATION_WEB_CONTENTS_TITLE_UPDATED) {
    std::pair<NavigationEntry*, bool>* title =
        content::Details<std::pair<NavigationEntry*, bool>>(details).ptr();

    if (title->first) {
      bool prevent_default = false;
      std::string text = UTF16ToUTF8(title->first->GetTitle());
      FOR_EACH_OBSERVER(NativeWindowObserver,
                        observers_,
                        OnPageTitleUpdated(&prevent_default, text));

      if (!prevent_default)
        SetTitle(text);
    }
  }
}

void NativeWindow::DevToolsSaveToFile(const std::string& url,
                                      const std::string& content,
                                      bool save_as) {
  base::FilePath path;
  PathsMap::iterator it = saved_files_.find(url);
  if (it != saved_files_.end() && !save_as) {
    path = it->second;
  } else {
    base::FilePath default_path(base::FilePath::FromUTF8Unsafe(url));
    if (!file_dialog::ShowSaveDialog(this, url, default_path, &path)) {
      base::StringValue url_value(url);
      CallDevToolsFunction("InspectorFrontendAPI.canceledSaveURL", &url_value);
      return;
    }
  }

  saved_files_[url] = path;
  base::WriteFile(path, content.data(), content.size());

  // Notify devtools.
  base::StringValue url_value(url);
  CallDevToolsFunction("InspectorFrontendAPI.savedURL", &url_value);
}

void NativeWindow::DevToolsAppendToFile(const std::string& url,
                                        const std::string& content) {
  PathsMap::iterator it = saved_files_.find(url);
  if (it == saved_files_.end())
    return;
  base::AppendToFile(it->second, content.data(), content.size());

  // Notify devtools.
  base::StringValue url_value(url);
  CallDevToolsFunction("InspectorFrontendAPI.appendedToURL", &url_value);
}

void NativeWindow::ScheduleUnresponsiveEvent(int ms) {
  if (!window_unresposive_closure_.IsCancelled())
    return;

  window_unresposive_closure_.Reset(
      base::Bind(&NativeWindow::NotifyWindowUnresponsive,
                 weak_factory_.GetWeakPtr()));
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      window_unresposive_closure_.callback(),
      base::TimeDelta::FromMilliseconds(ms));
}

void NativeWindow::NotifyWindowUnresponsive() {
  window_unresposive_closure_.Cancel();

  if (!is_closed_ && !HasModalDialog())
    FOR_EACH_OBSERVER(NativeWindowObserver,
                      observers_,
                      OnRendererUnresponsive());
}

void NativeWindow::OnCapturePageDone(const CapturePageCallback& callback,
                                     bool succeed,
                                     const SkBitmap& bitmap) {
  std::vector<unsigned char> data;
  if (succeed)
    gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, true, &data);
  callback.Run(data);
}

void NativeWindow::CallDevToolsFunction(const std::string& function_name,
                                        const base::Value* arg1,
                                        const base::Value* arg2,
                                        const base::Value* arg3) {
  std::string params;
  if (arg1) {
    std::string json;
    base::JSONWriter::Write(arg1, &json);
    params.append(json);
    if (arg2) {
      base::JSONWriter::Write(arg2, &json);
      params.append(", " + json);
      if (arg3) {
        base::JSONWriter::Write(arg3, &json);
        params.append(", " + json);
      }
    }
  }
  base::string16 javascript =
      base::UTF8ToUTF16(function_name + "(" + params + ");");
  GetDevToolsWebContents()->GetMainFrame()->ExecuteJavaScript(javascript);
}

}  // namespace atom
