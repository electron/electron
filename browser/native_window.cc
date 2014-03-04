// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/native_window.h"

#include <string>

#include "base/file_util.h"
#include "base/prefs/pref_service.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "browser/api/atom_browser_bindings.h"
#include "browser/atom_browser_context.h"
#include "browser/atom_browser_main_parts.h"
#include "browser/atom_javascript_dialog_manager.h"
#include "browser/browser.h"
#include "browser/devtools_delegate.h"
#include "browser/window_list.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/renderer_preferences.h"
#include "common/api/api_messages.h"
#include "common/atom_version.h"
#include "common/options_switches.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include "webkit/common/user_agent/user_agent_util.h"

using content::NavigationEntry;

namespace atom {

namespace {

const char kDockSidePref[] = "brightray.devtools.dockside";

}  // namespace

NativeWindow::NativeWindow(content::WebContents* web_contents,
                           base::DictionaryValue* options)
    : content::WebContentsObserver(web_contents),
      has_frame_(true),
      is_closed_(false),
      node_integration_("all"),
      has_dialog_attached_(false),
      weak_factory_(this),
      inspectable_web_contents_(
          brightray::InspectableWebContents::Create(web_contents)) {
  options->GetBoolean(switches::kFrame, &has_frame_);

  // Read icon before window is created.
  std::string icon;
  if (options->GetString(switches::kIcon, &icon) && !SetIcon(icon))
    LOG(ERROR) << "Failed to set icon to " << icon;

  // Read iframe security before any navigation.
  options->GetString(switches::kNodeIntegration, &node_integration_);

  web_contents->SetDelegate(this);
  inspectable_web_contents()->SetDelegate(this);

  WindowList::AddWindow(this);

  // Override the user agent to contain application and atom-shell's version.
  Browser* browser = Browser::Get();
  std::string product_name = base::StringPrintf(
      "%s/%s Atom-Shell/" ATOM_VERSION_STRING,
      browser->GetName().c_str(),
      browser->GetVersion().c_str());
  web_contents->GetMutableRendererPrefs()->user_agent_override =
      webkit_glue::BuildUserAgentFromProduct(product_name);

  // Get notified of title updated message.
  registrar_.Add(this, content::NOTIFICATION_WEB_CONTENTS_TITLE_UPDATED,
      content::Source<content::WebContents>(web_contents));
}

NativeWindow::~NativeWindow() {
  // It's possible that the windows gets destroyed before it's closed, in that
  // case we need to ensure the OnWindowClosed message is still notified.
  NotifyWindowClosed();
}

// static
NativeWindow* NativeWindow::Create(base::DictionaryValue* options) {
  content::WebContents::CreateParams create_params(AtomBrowserContext::Get());
  return Create(content::WebContents::Create(create_params), options);
}

// static
NativeWindow* NativeWindow::Debug(content::WebContents* web_contents) {
  base::DictionaryValue options;
  NativeWindow* window = NativeWindow::Create(&options);
  window->devtools_delegate_.reset(new DevToolsDelegate(window, web_contents));
  return window;
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

void NativeWindow::InitFromOptions(base::DictionaryValue* options) {
  // Setup window from options.
  int x = -1, y = -1;
  bool center;
  if (options->GetInteger(switches::kX, &x) &&
      options->GetInteger(switches::kY, &y)) {
    int width = -1, height = -1;
    options->GetInteger(switches::kWidth, &width);
    options->GetInteger(switches::kHeight, &height);
    Move(gfx::Rect(x, y, width, height));
  } else if (options->GetBoolean(switches::kCenter, &center) && center) {
    Center();
  }
  int min_height = -1, min_width = -1;
  if (options->GetInteger(switches::kMinHeight, &min_height) &&
      options->GetInteger(switches::kMinWidth, &min_width)) {
    SetMinimumSize(gfx::Size(min_width, min_height));
  }
  int max_height = -1, max_width = -1;
  if (options->GetInteger(switches::kMaxHeight, &max_height) &&
      options->GetInteger(switches::kMaxWidth, &max_width)) {
    SetMaximumSize(gfx::Size(max_width, max_height));
  }
  bool resizable;
  if (options->GetBoolean(switches::kResizable, &resizable)) {
    SetResizable(resizable);
  }
  bool top;
  if (options->GetBoolean(switches::kAlwaysOnTop, &top) && top) {
    SetAlwaysOnTop(true);
  }
  bool fullscreen;
  if (options->GetBoolean(switches::kFullscreen, &fullscreen) && fullscreen) {
    SetFullscreen(true);
  }
  bool kiosk;
  if (options->GetBoolean(switches::kKiosk, &kiosk) && kiosk) {
    SetKiosk(kiosk);
  }
  std::string title("Atom Shell");
  options->GetString(switches::kTitle, &title);
  SetTitle(title);

  // Then show it.
  bool show = true;
  options->GetBoolean(switches::kShow, &show);
  if (show)
    Show();
}

bool NativeWindow::HasModalDialog() {
  return has_dialog_attached_;
}

void NativeWindow::OpenDevTools() {
  // For docked devtools we give it to brightray.
  inspectable_web_contents()->ShowDevTools();
}

void NativeWindow::CloseDevTools() {
  inspectable_web_contents()->GetView()->CloseDevTools();
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
  return GetWebContents()->GetRenderViewHost()->GetView()->HasFocus();
}

bool NativeWindow::SetIcon(const std::string& str_path) {
  base::FilePath path = base::FilePath::FromUTF8Unsafe(str_path);

  // Read the file from disk.
  std::string file_contents;
  if (path.empty() || !base::ReadFileToString(path, &file_contents))
    return false;

  // Decode the bitmap using WebKit's image decoder.
  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(file_contents.data());
  scoped_ptr<SkBitmap> decoded(new SkBitmap());
  gfx::PNGCodec::Decode(data, file_contents.length(), decoded.get());
  if (decoded->empty())
    return false;  // Unable to decode.

  icon_ = gfx::Image::CreateFrom1xBitmap(*decoded.release());
  return true;
}

base::ProcessHandle NativeWindow::GetRenderProcessHandle() {
  return GetWebContents()->GetRenderProcessHost()->GetHandle();
}

void NativeWindow::CapturePage(const gfx::Rect& rect,
                               const CapturePageCallback& callback) {
  gfx::Rect flipped_y_rect = rect;
  flipped_y_rect.set_y(-rect.y());

  GetWebContents()->GetRenderViewHost()->CopyFromBackingStore(
      flipped_y_rect,
      gfx::Size(),
      base::Bind(&NativeWindow::OnCapturePageDone,
                 weak_factory_.GetWeakPtr(),
                 callback));
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

  // Assume the window is not responding if it doesn't cancel the close and is
  // not closed in 500ms, in this way we can quickly show the unresponsive
  // dialog when the window is busy executing some script withouth waiting for
  // the unresponsive timeout.
  if (!Browser::Get()->is_quiting() &&
      window_unresposive_closure_.IsCancelled()) {
    window_unresposive_closure_.Reset(
        base::Bind(&NativeWindow::RendererUnresponsive,
                   weak_factory_.GetWeakPtr(),
                   web_contents));
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        window_unresposive_closure_.callback(),
        base::TimeDelta::FromMilliseconds(500));
  }

  if (web_contents->NeedToFireBeforeUnload())
    web_contents->GetRenderViewHost()->FirePageBeforeUnload(false);
  else
    web_contents->Close();
}

content::WebContents* NativeWindow::GetWebContents() const {
  return inspectable_web_contents_->GetWebContents();
}

content::WebContents* NativeWindow::GetDevToolsWebContents() const {
  return inspectable_web_contents()->devtools_web_contents();
}

void NativeWindow::NotifyWindowClosed() {
  if (is_closed_)
    return;

  // The OnRenderViewDeleted is not called when the WebContents is destroyed
  // directly (e.g. when closing the window), so we make sure it's always
  // emitted to users by sending it before window is closed..
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnRenderViewDeleted(
                        GetWebContents()->GetRenderProcessHost()->GetID(),
                        GetWebContents()->GetRoutingID()));

  is_closed_ = true;
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowClosed());

  WindowList::RemoveWindow(this);
}

void NativeWindow::NotifyWindowBlur() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowBlur());
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

  if (!proceed)
    WindowList::WindowCloseCancelled(this);

  // When the "beforeunload" callback is fired the window is certainly live.
  window_unresposive_closure_.Cancel();
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

void NativeWindow::LoadingStateChanged(content::WebContents* source) {
  bool is_loading = source->IsLoading();
  FOR_EACH_OBSERVER(NativeWindowObserver,
                    observers_,
                    OnLoadingStateChanged(is_loading));
}

void NativeWindow::MoveContents(content::WebContents* source,
                                const gfx::Rect& pos) {
  SetPosition(pos.origin());
  SetSize(pos.size());
}

void NativeWindow::CloseContents(content::WebContents* source) {
  // When the web contents is gone, close the window immediately, but the
  // memory will not be freed until you call delete.
  // In this way, it would be safe to manage windows via smart pointers. If you
  // want to free memory when the window is closed, you can do deleting by
  // overriding the OnWindowClosed method in the observer.
  CloseImmediately();

  NotifyWindowClosed();

  // Do not sent "unresponsive" event after window is closed.
  window_unresposive_closure_.Cancel();
}

bool NativeWindow::IsPopupOrPanel(const content::WebContents* source) const {
  // Only popup window can use things like window.moveTo.
  return true;
}

void NativeWindow::RendererUnresponsive(content::WebContents* source) {
  window_unresposive_closure_.Cancel();

  if (!HasModalDialog())
    FOR_EACH_OBSERVER(NativeWindowObserver,
                      observers_,
                      OnRendererUnresponsive());
}

void NativeWindow::RendererResponsive(content::WebContents* source) {
  window_unresposive_closure_.Cancel();
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnRendererResponsive());
}

void NativeWindow::RenderViewDeleted(content::RenderViewHost* rvh) {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnRenderViewDeleted(rvh->GetProcess()->GetID(),
                                        rvh->GetRoutingID()));
}

void NativeWindow::RenderProcessGone(base::TerminationStatus status) {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnRendererCrashed());
}

void NativeWindow::BeforeUnloadFired(const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
}

bool NativeWindow::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NativeWindow, message)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AtomViewHostMsg_Message_Sync,
                                    OnRendererMessageSync)
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

bool NativeWindow::DevToolsSetDockSide(const std::string& dock_side,
                                       bool* succeed) {
  if (dock_side != "undocked")
    return false;

  CloseDevTools();
  Debug(GetWebContents());
  return true;
}

bool NativeWindow::DevToolsShow(const std::string& dock_side) {
  if (dock_side != "undocked")
    return false;

  Debug(GetWebContents());
  return true;
}

void NativeWindow::OnCapturePageDone(const CapturePageCallback& callback,
                                     bool succeed,
                                     const SkBitmap& bitmap) {
  std::vector<unsigned char> data;
  if (succeed)
    gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, true, &data);
  callback.Run(data);
}

void NativeWindow::OnRendererMessage(const string16& channel,
                                     const base::ListValue& args) {
  AtomBrowserMainParts::Get()->atom_bindings()->OnRendererMessage(
      GetWebContents()->GetRenderProcessHost()->GetID(),
      GetWebContents()->GetRoutingID(),
      channel,
      args);
}

void NativeWindow::OnRendererMessageSync(const string16& channel,
                                         const base::ListValue& args,
                                         IPC::Message* reply_msg) {
  AtomBrowserMainParts::Get()->atom_bindings()->OnRendererMessageSync(
      GetWebContents()->GetRenderProcessHost()->GetID(),
      GetWebContents()->GetRoutingID(),
      channel,
      args,
      this,
      reply_msg);
}

}  // namespace atom
