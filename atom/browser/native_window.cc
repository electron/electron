// Copyright (c) 2013 GitHub, Inc.
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
#include "atom/browser/web_dialog_helper.h"
#include "atom/browser/window_list.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/prefs/pref_service.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
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
#include "content/public/common/content_switches.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/user_agent.h"
#include "content/public/common/web_preferences.h"
#include "ipc/ipc_message_macros.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/screen.h"
#include "ui/gfx/size.h"

#if defined(OS_WIN)
#include "ui/gfx/switches.h"
#endif

using content::NavigationEntry;
using content::RenderWidgetHostView;
using content::RenderWidgetHost;

namespace content {
CONTENT_EXPORT extern bool g_use_transparent_window;
}

namespace atom {

namespace {

// Array of available web runtime features.
const char* kWebRuntimeFeatures[] = {
  switches::kExperimentalFeatures,
  switches::kExperimentalCanvasFeatures,
  switches::kSubpixelFontScaling,
  switches::kOverlayScrollbars,
  switches::kOverlayFullscreenVideo,
  switches::kSharedWorker,
};

std::string RemoveWhitespace(const std::string& str) {
  std::string trimmed;
  if (base::RemoveChars(str, " ", &trimmed))
    return trimmed;
  else
    return str;
}

}  // namespace

NativeWindow::NativeWindow(content::WebContents* web_contents,
                           const mate::Dictionary& options)
    : content::WebContentsObserver(web_contents),
      has_frame_(true),
      transparent_(false),
      enable_larger_than_screen_(false),
      is_closed_(false),
      node_integration_(true),
      has_dialog_attached_(false),
      zoom_factor_(1.0),
      weak_factory_(this),
      inspectable_web_contents_(
          brightray::InspectableWebContents::Create(web_contents)) {
  printing::PrintViewManagerBasic::CreateForWebContents(web_contents);

  options.Get(switches::kFrame, &has_frame_);
  options.Get(switches::kTransparent, &transparent_);
  options.Get(switches::kEnableLargerThanScreen, &enable_larger_than_screen_);
  options.Get(switches::kNodeIntegration, &node_integration_);

  // Tell the content module to initialize renderer widget with transparent
  // mode.
  content::g_use_transparent_window = transparent_;

  // Read icon before window is created.
  options.Get(switches::kIcon, &icon_);

  // The "preload" option must be absolute path.
  if (options.Get(switches::kPreloadScript, &preload_script_) &&
      !preload_script_.IsAbsolute()) {
    LOG(ERROR) << "Path of \"preload\" script must be absolute.";
    preload_script_.clear();
  }

  // Be compatible with old API of "node-integration" option.
  std::string old_string_token;
  if (options.Get(switches::kNodeIntegration, &old_string_token) &&
      old_string_token != "disable")
    node_integration_ = true;

  // Read the web preferences.
  options.Get(switches::kWebPreferences, &web_preferences_);

  // Read the zoom factor before any navigation.
  options.Get(switches::kZoomFactor, &zoom_factor_);

  web_contents->SetDelegate(this);
  inspectable_web_contents()->SetDelegate(this);

  WindowList::AddWindow(this);

  // Override the user agent to contain application and atom-shell's version.
  Browser* browser = Browser::Get();
  std::string product_name = base::StringPrintf(
      "%s/%s Chrome/%s AtomShell/" ATOM_VERSION_STRING,
      RemoveWhitespace(browser->GetName()).c_str(),
      browser->GetVersion().c_str(),
      CHROME_VERSION_STRING);
  web_contents->GetMutableRendererPrefs()->user_agent_override =
      content::BuildUserAgentFromProduct(product_name);

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

  return nullptr;
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
  int min_height = 0, min_width = 0;
  if (options.Get(switches::kMinHeight, &min_height) |
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
    SetFullScreen(true);
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

std::string NativeWindow::GetRepresentedFilename() {
  return "";
}

void NativeWindow::SetDocumentEdited(bool edited) {
}

bool NativeWindow::IsDocumentEdited() {
  return false;
}

void NativeWindow::SetMenu(ui::MenuModel* menu) {
}

void NativeWindow::Print(bool silent, bool print_background) {
  printing::PrintViewManagerBasic::FromWebContents(GetWebContents())->
      PrintNow(silent, print_background);
}

void NativeWindow::ShowDefinitionForSelection() {
  NOTIMPLEMENTED();
}

void NativeWindow::SetAutoHideMenuBar(bool auto_hide) {
}

bool NativeWindow::IsMenuBarAutoHide() {
  return false;
}

void NativeWindow::SetMenuBarVisibility(bool visible) {
}

bool NativeWindow::IsMenuBarVisible() {
  return true;
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
  scoped_refptr<content::DevToolsAgentHost> agent(
      content::DevToolsAgentHost::GetOrCreateFor(GetWebContents()));
  agent->InspectElement(x, y);
}

void NativeWindow::FocusOnWebView() {
  GetWebContents()->GetRenderViewHost()->Focus();
}

void NativeWindow::BlurWebView() {
  GetWebContents()->GetRenderViewHost()->Blur();
}

bool NativeWindow::IsWebViewFocused() {
  RenderWidgetHostView* host_view =
      GetWebContents()->GetRenderViewHost()->GetView();
  return host_view && host_view->HasFocus();
}

void NativeWindow::CapturePage(const gfx::Rect& rect,
                               const CapturePageCallback& callback) {
  content::WebContents* contents = GetWebContents();
  RenderWidgetHostView* const view = contents->GetRenderWidgetHostView();
  RenderWidgetHost* const host = view ? view->GetRenderWidgetHost() : nullptr;
  if (!view || !host) {
    callback.Run(std::vector<unsigned char>());
    return;
  }

  // Capture full page if user doesn't specify a |rect|.
  const gfx::Size view_size = rect.IsEmpty() ? view->GetViewBounds().size() :
                                               rect.size();

  // By default, the requested bitmap size is the view size in screen
  // coordinates.  However, if there's more pixel detail available on the
  // current system, increase the requested bitmap size to capture it all.
  gfx::Size bitmap_size = view_size;
  const gfx::NativeView native_view = view->GetNativeView();
  gfx::Screen* const screen = gfx::Screen::GetScreenFor(native_view);
  const float scale =
      screen->GetDisplayNearestWindow(native_view).device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ToCeiledSize(gfx::ScaleSize(view_size, scale));

  host->CopyFromBackingStore(
      rect.IsEmpty() ? gfx::Rect(view_size) : rect,
      bitmap_size,
      base::Bind(&NativeWindow::OnCapturePageDone,
                 weak_factory_.GetWeakPtr(),
                 callback),
      kBGRA_8888_SkColorType);
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
    web_contents->DispatchBeforeUnload(false);
  else
    web_contents->Close();
}

content::WebContents* NativeWindow::GetWebContents() const {
  if (!inspectable_web_contents_)
    return nullptr;
  return inspectable_web_contents()->GetWebContents();
}

content::WebContents* NativeWindow::GetDevToolsWebContents() const {
  if (!inspectable_web_contents_)
    return nullptr;
  return inspectable_web_contents()->devtools_web_contents();
}

void NativeWindow::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line, int child_process_id) {
  // Append --node-integration to renderer process.
  command_line->AppendSwitchASCII(switches::kNodeIntegration,
                                  node_integration_ ? "true" : "false");

  // Append --preload.
  if (!preload_script_.empty())
    command_line->AppendSwitchPath(switches::kPreloadScript, preload_script_);

  // Append --zoom-factor.
  if (zoom_factor_ != 1.0)
    command_line->AppendSwitchASCII(switches::kZoomFactor,
                                    base::DoubleToString(zoom_factor_));

  if (web_preferences_.IsEmpty())
    return;

  bool b;
#if defined(OS_WIN)
  // Check if DirectWrite is disabled.
  if (web_preferences_.Get(switches::kDirectWrite, &b) && !b)
    command_line->AppendSwitch(::switches::kDisableDirectWrite);
#endif

  // Check if plugins are enabled.
  if (web_preferences_.Get("plugins", &b) && b)
    command_line->AppendSwitch(switches::kEnablePlugins);

  // This set of options are not availabe in WebPreferences, so we have to pass
  // them via command line and enable them in renderer procss.
  for (size_t i = 0; i < arraysize(kWebRuntimeFeatures); ++i) {
    const char* feature = kWebRuntimeFeatures[i];
    if (web_preferences_.Get(feature, &b))
      command_line->AppendSwitchASCII(feature, b ? "true" : "false");
  }
}

void NativeWindow::OverrideWebkitPrefs(const GURL& url,
                                       content::WebPreferences* prefs) {
  if (web_preferences_.IsEmpty())
    return;

  bool b;
  std::vector<base::FilePath> list;
  if (web_preferences_.Get("javascript", &b))
    prefs->javascript_enabled = b;
  if (web_preferences_.Get("web-security", &b))
    prefs->web_security_enabled = b;
  if (web_preferences_.Get("images", &b))
    prefs->images_enabled = b;
  if (web_preferences_.Get("java", &b))
    prefs->java_enabled = b;
  if (web_preferences_.Get("text-areas-are-resizable", &b))
    prefs->text_areas_are_resizable = b;
  if (web_preferences_.Get("webgl", &b))
    prefs->experimental_webgl_enabled = b;
  if (web_preferences_.Get("webaudio", &b))
    prefs->webaudio_enabled = b;
  if (web_preferences_.Get("extra-plugin-dirs", &list)) {
    for (size_t i = 0; i < list.size(); ++i)
      content::PluginService::GetInstance()->AddExtraPluginDir(list[i]);
  }
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

void NativeWindow::NotifyWindowMaximize() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowMaximize());
}

void NativeWindow::NotifyWindowUnmaximize() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowUnmaximize());
}

void NativeWindow::NotifyWindowMinimize() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowMinimize());
}

void NativeWindow::NotifyWindowRestore() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowRestore());
}

void NativeWindow::NotifyWindowEnterFullScreen() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnWindowEnterFullScreen());
}

void NativeWindow::NotifyWindowLeaveFullScreen() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnWindowLeaveFullScreen());
}

bool NativeWindow::ShouldCreateWebContents(
    content::WebContents* web_contents,
    int route_id,
    WindowContainerType window_container_type,
    const base::string16& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  FOR_EACH_OBSERVER(NativeWindowObserver,
                    observers_,
                    WillCreatePopupWindow(frame_name,
                                          target_url,
                                          partition_id,
                                          NEW_FOREGROUND_TAB));
  return false;
}

// In atom-shell all reloads and navigations started by renderer process would
// be redirected to this method, so we can have precise control of how we
// would open the url (in our case, is to restart the renderer process). See
// AtomRendererClient::ShouldFork for how this is done.
content::WebContents* NativeWindow::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (params.disposition != CURRENT_TAB) {
    FOR_EACH_OBSERVER(NativeWindowObserver,
                      observers_,
                      WillCreatePopupWindow(base::string16(),
                                            params.url,
                                            "",
                                            params.disposition));
    return nullptr;
  }

  // Give user a chance to prevent navigation.
  bool prevent_default = false;
  FOR_EACH_OBSERVER(NativeWindowObserver,
                    observers_,
                    WillNavigate(&prevent_default, params.url));
  if (prevent_default)
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

  source->GetController().LoadURLWithParams(load_url_params);
  return source;
}

content::JavaScriptDialogManager* NativeWindow::GetJavaScriptDialogManager() {
  if (!dialog_manager_)
    dialog_manager_.reset(new AtomJavaScriptDialogManager);

  return dialog_manager_.get();
}

void NativeWindow::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  if (!transparent_)
    return;

  content::RenderWidgetHostImpl* impl = content::RenderWidgetHostImpl::FromID(
      render_view_host->GetProcess()->GetID(),
      render_view_host->GetRoutingID());
  if (impl)
    impl->SetBackgroundOpaque(false);
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

content::ColorChooser* NativeWindow::OpenColorChooser(
    content::WebContents* web_contents,
    SkColor color,
    const std::vector<content::ColorSuggestion>& suggestions) {
  return chrome::ShowColorChooser(web_contents, color);
}

void NativeWindow::RunFileChooser(content::WebContents* web_contents,
                                  const content::FileChooserParams& params) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(this));
  web_dialog_helper_->RunFileChooser(web_contents, params);
}

void NativeWindow::EnumerateDirectory(content::WebContents* web_contents,
                                      int request_id,
                                      const base::FilePath& path) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(this));
  web_dialog_helper_->EnumerateDirectory(web_contents, request_id, path);
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
      std::string text = base::UTF16ToUTF8(title->first->GetTitle());
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
    file_dialog::Filters filters;
    base::FilePath default_path(base::FilePath::FromUTF8Unsafe(url));
    if (!file_dialog::ShowSaveDialog(this, url, default_path, filters, &path)) {
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
  SkAutoLockPixels screen_capture_lock(bitmap);
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
