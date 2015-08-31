// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window.h"

#include <string>
#include <utility>
#include <vector>

#include "content/public/browser/render_widget_host_view_frame_subscriber.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "media/base/video_frame.h"
#include "media/base/yuv_convert.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/window_list.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/prefs/pref_service.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/web_preferences.h"
#include "ipc/ipc_message_macros.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/screen.h"
#include "ui/gl/gpu_switching_manager.h"

#if defined(OS_WIN)
#include "ui/gfx/switches.h"
#endif

using content::NavigationEntry;
using content::RenderWidgetHostView;
using content::RenderWidgetHost;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::NativeWindowRelay);

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
  switches::kPageVisibility,
};

// Convert draggable regions in raw format to SkRegion format. Caller is
// responsible for deleting the returned SkRegion instance.
scoped_ptr<SkRegion> DraggableRegionsToSkRegion(
    const std::vector<DraggableRegion>& regions) {
  scoped_ptr<SkRegion> sk_region(new SkRegion);
  for (const DraggableRegion& region : regions) {
    sk_region->op(
        region.bounds.x(),
        region.bounds.y(),
        region.bounds.right(),
        region.bounds.bottom(),
        region.draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }
  return sk_region.Pass();
}

}  // namespace

NativeWindow::NativeWindow(
    brightray::InspectableWebContents* inspectable_web_contents,
    const mate::Dictionary& options)
    : content::WebContentsObserver(inspectable_web_contents->GetWebContents()),
      has_frame_(true),
      transparent_(false),
      enable_larger_than_screen_(false),
      is_closed_(false),
      node_integration_(true),
      has_dialog_attached_(false),
      zoom_factor_(1.0),
      aspect_ratio_(0.0),
      inspectable_web_contents_(inspectable_web_contents),
      weak_factory_(this) {
  inspectable_web_contents->GetView()->SetDelegate(this);

  options.Get(switches::kFrame, &has_frame_);
  options.Get(switches::kTransparent, &transparent_);
  options.Get(switches::kEnableLargerThanScreen, &enable_larger_than_screen_);
  options.Get(switches::kNodeIntegration, &node_integration_);

  // Tell the content module to initialize renderer widget with transparent
  // mode.
  ui::GpuSwitchingManager::SetTransparent(transparent_);

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

  WindowList::AddWindow(this);
}

NativeWindow::~NativeWindow() {
  // It's possible that the windows gets destroyed before it's closed, in that
  // case we need to ensure the OnWindowClosed message is still notified.
  NotifyWindowClosed();
}

// static
NativeWindow* NativeWindow::FromWebContents(
    content::WebContents* web_contents) {
  WindowList& window_list = *WindowList::GetInstance();
  for (NativeWindow* window : window_list) {
    if (window->web_contents() == web_contents){
      return window;
    }
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
    SetBounds(gfx::Rect(x, y, width, height));
  } else if (options.Get(switches::kCenter, &center) && center) {
    Center();
  }
  int min_height = 0, min_width = 0;
  if (options.Get(switches::kMinHeight, &min_height) |
      options.Get(switches::kMinWidth, &min_width)) {
    SetMinimumSize(gfx::Size(min_width, min_height));
  }
  int max_height = INT_MAX, max_width = INT_MAX;
  if (options.Get(switches::kMaxHeight, &max_height) |
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
#if defined(OS_MACOSX) || defined(OS_WIN)
  bool fullscreen;
  if (options.Get(switches::kFullscreen, &fullscreen) && fullscreen) {
    SetFullScreen(true);
  }
#endif
  bool skip;
  if (options.Get(switches::kSkipTaskbar, &skip) && skip) {
    SetSkipTaskbar(skip);
  }
  bool kiosk;
  if (options.Get(switches::kKiosk, &kiosk) && kiosk) {
    SetKiosk(kiosk);
  }
  std::string title("Electron");
  options.Get(switches::kTitle, &title);
  SetTitle(title);

  offscreen_ = false;
  options.Get(switches::kOffScreenRender, &offscreen_);

  // Then show it.
  bool show = true;
  options.Get(switches::kShow, &show);
  if (show)
    Show();
}

void NativeWindow::SetSize(const gfx::Size& size) {
  SetBounds(gfx::Rect(GetPosition(), size));
}

gfx::Size NativeWindow::GetSize() {
  return GetBounds().size();
}

void NativeWindow::SetPosition(const gfx::Point& position) {
  SetBounds(gfx::Rect(position, GetSize()));
}

gfx::Point NativeWindow::GetPosition() {
  return GetBounds().origin();
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

double NativeWindow::GetAspectRatio() {
  return aspect_ratio_;
}

gfx::Size NativeWindow::GetAspectRatioExtraSize() {
  return aspect_ratio_extraSize_;
}

void NativeWindow::SetAspectRatio(double aspect_ratio,
                                  const gfx::Size& extra_size) {
  aspect_ratio_ = aspect_ratio;
  aspect_ratio_extraSize_ = extra_size;
}

bool NativeWindow::HasModalDialog() {
  return has_dialog_attached_;
}

void NativeWindow::FocusOnWebView() {
  web_contents()->GetRenderViewHost()->Focus();
}

void NativeWindow::BlurWebView() {
  web_contents()->GetRenderViewHost()->Blur();
}

bool NativeWindow::IsWebViewFocused() {
  auto host_view = web_contents()->GetRenderViewHost()->GetView();
  return host_view && host_view->HasFocus();
}

void NativeWindow::CapturePage(const gfx::Rect& rect,
                               const CapturePageCallback& callback) {
  const auto view = web_contents()->GetRenderWidgetHostView();
  const auto host = view ? view->GetRenderWidgetHost() : nullptr;
  if (!view || !host) {
    callback.Run(SkBitmap());
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

void NativeWindow::SetOffscreenRender(bool isOffscreen) {
  if (!isOffscreen && !offscreen_) return;

  const auto view = web_contents()->GetRenderWidgetHostView();

  if (view) {
    if (isOffscreen) {
      scoped_ptr<content::RenderWidgetHostViewFrameSubscriber> subscriber(
        new RenderSubscriber(
          view->GetVisibleViewportSize(),
          base::Bind(&NativeWindow::OnFrameReceived, base::Unretained(this))
        )
      );

      view->BeginFrameSubscription(subscriber.Pass());
    } else {
      view->EndFrameSubscription();
    }

    offscreen_ = isOffscreen;
  }
}

void NativeWindow::RequestToClosePage() {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(NativeWindowObserver,
                    observers_,
                    WillCloseWindow(&prevent_default));
  if (prevent_default) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

  // Assume the window is not responding if it doesn't cancel the close and is
  // not closed in 5s, in this way we can quickly show the unresponsive
  // dialog when the window is busy executing some script withouth waiting for
  // the unresponsive timeout.
  if (window_unresposive_closure_.IsCancelled())
    ScheduleUnresponsiveEvent(5000);

  if (web_contents()->NeedToFireBeforeUnload())
    web_contents()->DispatchBeforeUnload(false);
  else
    web_contents()->Close();
}

void NativeWindow::CloseContents(content::WebContents* source) {
  if (!inspectable_web_contents_)
    return;

  inspectable_web_contents_->GetView()->SetDelegate(nullptr);
  inspectable_web_contents_ = nullptr;
  Observe(nullptr);

  // When the web contents is gone, close the window immediately, but the
  // memory will not be freed until you call delete.
  // In this way, it would be safe to manage windows via smart pointers. If you
  // want to free memory when the window is closed, you can do deleting by
  // overriding the OnWindowClosed method in the observer.
  CloseImmediately();

  // Do not sent "unresponsive" event after window is closed.
  window_unresposive_closure_.Cancel();
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

void NativeWindow::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line) {
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

void NativeWindow::OverrideWebkitPrefs(content::WebPreferences* prefs) {
  if (web_preferences_.IsEmpty())
    return;

  bool b;
  std::vector<base::FilePath> list;
  if (web_preferences_.Get("javascript", &b))
    prefs->javascript_enabled = b;
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
  if (web_preferences_.Get("web-security", &b)) {
    prefs->web_security_enabled = b;
    prefs->allow_displaying_insecure_content = !b;
    prefs->allow_running_insecure_content = !b;
  }
  if (web_preferences_.Get("allow-displaying-insecure-content", &b))
    prefs->allow_displaying_insecure_content = b;
  if (web_preferences_.Get("allow-running-insecure-content", &b))
    prefs->allow_running_insecure_content = b;
  if (web_preferences_.Get("extra-plugin-dirs", &list)) {
    if (content::PluginService::GetInstance()->NPAPIPluginsSupported()) {
      for (size_t i = 0; i < list.size(); ++i)
        content::PluginService::GetInstance()->AddExtraPluginDir(list[i]);
    } else {
      LOG(WARNING) << "NPAPI plugins not supported on this platform";
    }
  }
}

void NativeWindow::NotifyWindowClosed() {
  if (is_closed_)
    return;

  WindowList::RemoveWindow(this);

  is_closed_ = true;
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowClosed());
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

void NativeWindow::NotifyWindowResize() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowResize());
}

void NativeWindow::NotifyWindowMove() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowMove());
}

void NativeWindow::NotifyWindowMoved() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowMoved());
}

void NativeWindow::NotifyWindowEnterFullScreen() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnWindowEnterFullScreen());
}

void NativeWindow::NotifyWindowLeaveFullScreen() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnWindowLeaveFullScreen());
}

void NativeWindow::NotifyWindowEnterHtmlFullScreen() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnWindowEnterHtmlFullScreen());
}

void NativeWindow::NotifyWindowLeaveHtmlFullScreen() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnWindowLeaveHtmlFullScreen());
}

void NativeWindow::NotifyWindowExecuteWindowsCommand(
    const std::string& command) {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_,
                    OnExecuteWindowsCommand(command));
}

void NativeWindow::DevToolsFocused() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnDevToolsFocus());
}

void NativeWindow::DevToolsOpened() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnDevToolsOpened());
}

void NativeWindow::DevToolsClosed() {
  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnDevToolsClosed());
}

void NativeWindow::SendKeyboardEvent(blink::WebInputEvent::Type type, int modifiers, int keycode){
  auto keyb_event = new content::NativeWebKeyboardEvent;

  keyb_event->nativeKeyCode = keycode;
  keyb_event->windowsKeyCode = keycode;
  keyb_event->setKeyIdentifierFromWindowsKeyCode();
  keyb_event->type = type;
  keyb_event->modifiers = modifiers;
  keyb_event->isSystemKey = false;
  keyb_event->timeStampSeconds = base::Time::Now().ToDoubleT();
  keyb_event->skip_in_browser = false;

  if (type == blink::WebInputEvent::Char || type == blink::WebInputEvent::KeyDown) {
    keyb_event->text[0] = keycode;
    keyb_event->unmodifiedText[0] = keycode;
  }

  const auto view = web_contents()->GetRenderWidgetHostView();
  const auto host = view ? view->GetRenderWidgetHost() : nullptr;
  host->ForwardKeyboardEvent(*keyb_event);
}

void NativeWindow::SendMouseEvent(blink::WebInputEvent::Type type, int modifiers, blink::WebMouseEvent::Button button, int x, int y, int movementX, int movementY, int clickCount){
  auto mouse_event = new blink::WebMouseEvent();

  mouse_event->x = x;
  mouse_event->y = y;
  mouse_event->windowX = x;
  mouse_event->windowY = y;
  mouse_event->clickCount = clickCount;
  mouse_event->type = type;
  mouse_event->modifiers = modifiers;
  mouse_event->button = button;

  mouse_event->timeStampSeconds = base::Time::Now().ToDoubleT();

  const auto view = web_contents()->GetRenderWidgetHostView();
  const auto host = view ? view->GetRenderWidgetHost() : nullptr;
  host->ForwardMouseEvent(*mouse_event);
}

void NativeWindow::SendMouseWheelEvent(int modifiers, int x, int y, bool precise){
  auto wheel_event = new blink::WebMouseWheelEvent();

  wheel_event->type = blink::WebInputEvent::MouseWheel;
  wheel_event->deltaX = x;
  wheel_event->deltaY = y;
  if(x) wheel_event->wheelTicksX = x > 0.0f ? 1.0f : -1.0f;
  if(y) wheel_event->wheelTicksY = y > 0.0f ? 1.0f : -1.0f;
  wheel_event->modifiers = modifiers;
  wheel_event->hasPreciseScrollingDeltas = precise;
  wheel_event->canScroll = true;

  const auto view = web_contents()->GetRenderWidgetHostView();
  const auto host = view ? view->GetRenderWidgetHost() : nullptr;
  host->ForwardWheelEvent(*wheel_event);
}

void NativeWindow::DidFinishLoad(content::RenderFrameHost* render_frame_host, const GURL& validated_url) {
  SetOffscreenRender(offscreen_);
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

void NativeWindow::BeforeUnloadDialogCancelled() {
  WindowList::WindowCloseCancelled(this);

  // Cancel unresponsive event when window close is cancelled.
  window_unresposive_closure_.Cancel();
}

void NativeWindow::TitleWasSet(content::NavigationEntry* entry,
                               bool explicit_set) {
  bool prevent_default = false;
  std::string text = entry ? base::UTF16ToUTF8(entry->GetTitle()) : "";
  FOR_EACH_OBSERVER(NativeWindowObserver,
                    observers_,
                    OnPageTitleUpdated(&prevent_default, text));
  if (!prevent_default)
    SetTitle(text);
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

void NativeWindow::UpdateDraggableRegions(
    const std::vector<DraggableRegion>& regions) {
  // Draggable region is not supported for non-frameless window.
  if (has_frame_)
    return;
  draggable_region_ = DraggableRegionsToSkRegion(regions);
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
                                     const SkBitmap& bitmap,
                                     content::ReadbackResponse response) {
  callback.Run(bitmap);
}

void NativeWindow::OnFrameReceived(bool result,
                                   scoped_refptr<media::VideoFrame> frame) {
  if (result) {
    gfx::Rect rect = frame->visible_rect();

    const int rgb_arr_size = rect.width() * rect.height() * 4;
    scoped_ptr<uint8[]> rgb_bytes(new uint8[rgb_arr_size]);

    // Convert a frame of YUV to 32 bit ARGB.
    media::ConvertYUVToRGB32(frame->data(media::VideoFrame::kYPlane),
                             frame->data(media::VideoFrame::kUPlane),
                             frame->data(media::VideoFrame::kVPlane),
                             rgb_bytes.get(),
                             rect.width(), rect.height(),
                             frame->stride(media::VideoFrame::kYPlane),
                             frame->stride(media::VideoFrame::kUVPlane),
                             rect.width() * 4,
                             media::YV12);

    FOR_EACH_OBSERVER(NativeWindowObserver,
                      observers_,
                      OnFrameRendered(rgb_bytes.Pass(), rgb_arr_size));
  }
}

bool RenderSubscriber::ShouldCaptureFrame(
                        const gfx::Rect& damage_rect,
                        base::TimeTicks present_time,
                        scoped_refptr<media::VideoFrame>* storage,
                        DeliverFrameCallback* callback) {
  last_present_time_ = present_time;
  *storage = media::VideoFrame::CreateFrame(media::VideoFrame::YV12, size_,
                                            gfx::Rect(size_), size_,
                                            base::TimeDelta());

  *callback = base::Bind(&RenderSubscriber::CallbackMethod, callback_, *storage);
  return true;
}

}  // namespace atom
