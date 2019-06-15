// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_browser_window.h"

#include "atom/browser/browser.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/window_list.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/api/constructor.h"
#include "atom/common/color_util.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "native_mate/dictionary.h"
#include "ui/gl/gpu_switching_manager.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

BrowserWindow::BrowserWindow(v8::Isolate* isolate,
                             v8::Local<v8::Object> wrapper,
                             const mate::Dictionary& options)
    : TopLevelWindow(isolate, options), weak_factory_(this) {
  mate::Handle<class WebContents> web_contents;

  // Use options.webPreferences in WebContents.
  mate::Dictionary web_preferences = mate::Dictionary::CreateEmpty(isolate);
  options.Get(options::kWebPreferences, &web_preferences);

  // Copy the backgroundColor to webContents.
  v8::Local<v8::Value> value;
  if (options.Get(options::kBackgroundColor, &value))
    web_preferences.Set(options::kBackgroundColor, value);

  v8::Local<v8::Value> transparent;
  if (options.Get("transparent", &transparent))
    web_preferences.Set("transparent", transparent);

  if (options.Get("webContents", &web_contents) && !web_contents.IsEmpty()) {
    // Set webPreferences from options if using an existing webContents.
    // These preferences will be used when the webContent launches new
    // render processes.
    auto* existing_preferences =
        WebContentsPreferences::From(web_contents->web_contents());
    base::DictionaryValue web_preferences_dict;
    if (mate::ConvertFromV8(isolate, web_preferences.GetHandle(),
                            &web_preferences_dict)) {
      existing_preferences->dict()->Clear();
      existing_preferences->Merge(web_preferences_dict);
    }
  } else {
    // Creates the WebContents used by BrowserWindow.
    web_contents = WebContents::Create(isolate, web_preferences);
  }

  web_contents_.Reset(isolate, web_contents.ToV8());
  api_web_contents_ = web_contents->GetWeakPtr();
  api_web_contents_->AddObserver(this);
  Observe(api_web_contents_->web_contents());

  // Keep a copy of the options for later use.
  mate::Dictionary(isolate, web_contents->GetWrapper())
      .Set("browserWindowOptions", options);

  // Tell the content module to initialize renderer widget with transparent
  // mode.
  ui::GpuSwitchingManager::SetTransparent(window()->transparent());

  // Associate with BrowserWindow.
  web_contents->SetOwnerWindow(window());

  auto* host = web_contents->web_contents()->GetRenderViewHost();
  if (host)
    host->GetWidget()->AddInputEventObserver(this);

  InitWith(isolate, wrapper);

#if defined(OS_MACOSX)
  if (!window()->has_frame())
    OverrideNSWindowContentView(web_contents->managed_web_contents());
#endif

  // Init window after everything has been setup.
  window()->InitFromOptions(options);
}

BrowserWindow::~BrowserWindow() {
  // FIXME This is a hack rather than a proper fix preventing shutdown crashes.
  if (api_web_contents_)
    api_web_contents_->RemoveObserver(this);
  // Note that the OnWindowClosed will not be called after the destructor runs,
  // since the window object is managed by the TopLevelWindow class.
  if (web_contents())
    Cleanup();
}

void BrowserWindow::OnInputEvent(const blink::WebInputEvent& event) {
  switch (event.GetType()) {
    case blink::WebInputEvent::kGestureScrollBegin:
    case blink::WebInputEvent::kGestureScrollUpdate:
    case blink::WebInputEvent::kGestureScrollEnd:
      Emit("scroll-touch-edge");
      break;
    default:
      break;
  }
}

void BrowserWindow::RenderViewHostChanged(content::RenderViewHost* old_host,
                                          content::RenderViewHost* new_host) {
  if (old_host)
    old_host->GetWidget()->RemoveInputEventObserver(this);
  if (new_host)
    new_host->GetWidget()->AddInputEventObserver(this);
}

void BrowserWindow::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  if (!window()->transparent())
    return;

  content::RenderWidgetHostImpl* impl = content::RenderWidgetHostImpl::FromID(
      render_view_host->GetProcess()->GetID(),
      render_view_host->GetRoutingID());
  if (impl)
    impl->SetBackgroundOpaque(false);
}

void BrowserWindow::DidFirstVisuallyNonEmptyPaint() {
  if (window()->IsVisible())
    return;

  // When there is a non-empty first paint, resize the RenderWidget to force
  // Chromium to draw.
  auto* const view = web_contents()->GetRenderWidgetHostView();
  view->Show();
  view->SetSize(window()->GetContentSize());

  // Emit the ReadyToShow event in next tick in case of pending drawing work.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<BrowserWindow> self) {
                       if (self)
                         self->Emit("ready-to-show");
                     },
                     GetWeakPtr()));
}

void BrowserWindow::BeforeUnloadDialogCancelled() {
  WindowList::WindowCloseCancelled(window());
  // Cancel unresponsive event when window close is cancelled.
  window_unresponsive_closure_.Cancel();
}

void BrowserWindow::OnRendererUnresponsive(content::RenderProcessHost*) {
  // Schedule the unresponsive shortly later, since we may receive the
  // responsive event soon. This could happen after the whole application had
  // blocked for a while.
  // Also notice that when closing this event would be ignored because we have
  // explicitly started a close timeout counter. This is on purpose because we
  // don't want the unresponsive event to be sent too early when user is closing
  // the window.
  ScheduleUnresponsiveEvent(50);
}

bool BrowserWindow::OnMessageReceived(const IPC::Message& message,
                                      content::RenderFrameHost* rfh) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(BrowserWindow, message, rfh)
    IPC_MESSAGE_HANDLER(AtomFrameHostMsg_UpdateDraggableRegions,
                        UpdateDraggableRegions)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BrowserWindow::OnCloseContents() {
  // On some machines it may happen that the window gets destroyed for twice,
  // checking web_contents() can effectively guard against that.
  // https://github.com/electron/electron/issues/16202.
  //
  // TODO(zcbenz): We should find out the root cause and improve the closing
  // procedure of BrowserWindow.
  if (!web_contents())
    return;

  // Close all child windows before closing current window.
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  for (v8::Local<v8::Value> value : child_windows_.Values(isolate())) {
    mate::Handle<BrowserWindow> child;
    if (mate::ConvertFromV8(isolate(), value, &child) && !child.IsEmpty())
      child->window()->CloseImmediately();
  }

  // When the web contents is gone, close the window immediately, but the
  // memory will not be freed until you call delete.
  // In this way, it would be safe to manage windows via smart pointers. If you
  // want to free memory when the window is closed, you can do deleting by
  // overriding the OnWindowClosed method in the observer.
  window()->CloseImmediately();

  // Do not sent "unresponsive" event after window is closed.
  window_unresponsive_closure_.Cancel();
}

void BrowserWindow::OnRendererResponsive() {
  window_unresponsive_closure_.Cancel();
  Emit("responsive");
}

void BrowserWindow::RequestPreferredWidth(int* width) {
  *width = web_contents()->GetPreferredSize().width();
}

void BrowserWindow::OnCloseButtonClicked(bool* prevent_default) {
  // When user tries to close the window by clicking the close button, we do
  // not close the window immediately, instead we try to close the web page
  // first, and when the web page is closed the window will also be closed.
  *prevent_default = true;

  // Assume the window is not responding if it doesn't cancel the close and is
  // not closed in 5s, in this way we can quickly show the unresponsive
  // dialog when the window is busy executing some script withouth waiting for
  // the unresponsive timeout.
  if (window_unresponsive_closure_.IsCancelled())
    ScheduleUnresponsiveEvent(5000);

  if (!web_contents())
    // Already closed by renderer
    return;

  if (web_contents()->NeedToFireBeforeUnload())
    web_contents()->DispatchBeforeUnload();
  else
    web_contents()->Close();
}

void BrowserWindow::OnWindowClosed() {
  Cleanup();
  TopLevelWindow::OnWindowClosed();
}

void BrowserWindow::OnWindowBlur() {
  web_contents()->StoreFocus();
#if defined(OS_MACOSX)
  auto* rwhv = web_contents()->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(false);
#endif

  TopLevelWindow::OnWindowBlur();
}

void BrowserWindow::OnWindowFocus() {
  web_contents()->RestoreFocus();
#if defined(OS_MACOSX)
  auto* rwhv = web_contents()->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(true);
#else
  if (!api_web_contents_->IsDevToolsOpened())
    web_contents()->Focus();
#endif

  TopLevelWindow::OnWindowFocus();
}

void BrowserWindow::OnWindowResize() {
#if defined(OS_MACOSX)
  if (!draggable_regions_.empty())
    UpdateDraggableRegions(nullptr, draggable_regions_);
#endif
  TopLevelWindow::OnWindowResize();
}

void BrowserWindow::OnWindowLeaveFullScreen() {
  TopLevelWindow::OnWindowLeaveFullScreen();
#if defined(OS_MACOSX)
  if (web_contents()->IsFullscreenForCurrentTab())
    web_contents()->ExitFullscreen(true);
#endif
}

void BrowserWindow::Focus() {
  if (api_web_contents_->IsOffScreen())
    FocusOnWebView();
  else
    TopLevelWindow::Focus();
}

void BrowserWindow::Blur() {
  if (api_web_contents_->IsOffScreen())
    BlurWebView();
  else
    TopLevelWindow::Blur();
}

void BrowserWindow::SetBackgroundColor(const std::string& color_name) {
  TopLevelWindow::SetBackgroundColor(color_name);
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->SetBackgroundColor(ParseHexColor(color_name));
}

void BrowserWindow::SetBrowserView(v8::Local<v8::Value> value) {
  TopLevelWindow::SetBrowserView(value);
#if defined(OS_MACOSX)
  UpdateDraggableRegions(nullptr, draggable_regions_);
#endif
}

void BrowserWindow::SetVibrancy(v8::Isolate* isolate,
                                v8::Local<v8::Value> value) {
  std::string type = mate::V8ToString(value);

  auto* render_view_host = web_contents()->GetRenderViewHost();
  if (render_view_host) {
    auto* impl = content::RenderWidgetHostImpl::FromID(
        render_view_host->GetProcess()->GetID(),
        render_view_host->GetRoutingID());
    if (impl)
      impl->SetBackgroundOpaque(type.empty() ? !window_->transparent() : false);
  }

  TopLevelWindow::SetVibrancy(isolate, value);
}

void BrowserWindow::FocusOnWebView() {
  web_contents()->GetRenderViewHost()->GetWidget()->Focus();
}

void BrowserWindow::BlurWebView() {
  web_contents()->GetRenderViewHost()->GetWidget()->Blur();
}

bool BrowserWindow::IsWebViewFocused() {
  auto* host_view = web_contents()->GetRenderViewHost()->GetWidget()->GetView();
  return host_view && host_view->HasFocus();
}

v8::Local<v8::Value> BrowserWindow::GetWebContents(v8::Isolate* isolate) {
  if (web_contents_.IsEmpty())
    return v8::Null(isolate);
  return v8::Local<v8::Value>::New(isolate, web_contents_);
}

// Convert draggable regions in raw format to SkRegion format.
std::unique_ptr<SkRegion> BrowserWindow::DraggableRegionsToSkRegion(
    const std::vector<DraggableRegion>& regions) {
  auto sk_region = std::make_unique<SkRegion>();
  for (const DraggableRegion& region : regions) {
    sk_region->op(
        region.bounds.x(), region.bounds.y(), region.bounds.right(),
        region.bounds.bottom(),
        region.draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }
  return sk_region;
}

void BrowserWindow::ScheduleUnresponsiveEvent(int ms) {
  if (!window_unresponsive_closure_.IsCancelled())
    return;

  window_unresponsive_closure_.Reset(
      base::Bind(&BrowserWindow::NotifyWindowUnresponsive, GetWeakPtr()));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, window_unresponsive_closure_.callback(),
      base::TimeDelta::FromMilliseconds(ms));
}

void BrowserWindow::NotifyWindowUnresponsive() {
  window_unresponsive_closure_.Cancel();
  if (!window_->IsClosed() && window_->IsEnabled() &&
      !IsUnresponsiveEventSuppressed()) {
    Emit("unresponsive");
  }
}

void BrowserWindow::Cleanup() {
  auto* host = web_contents()->GetRenderViewHost();
  if (host)
    host->GetWidget()->RemoveInputEventObserver(this);

  api_web_contents_->DestroyWebContents(true /* async */);
  Observe(nullptr);
}

// static
mate::WrappableBase* BrowserWindow::New(mate::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    args->ThrowError("Cannot create BrowserWindow before app is ready");
    return nullptr;
  }

  if (args->Length() > 1) {
    args->ThrowError();
    return nullptr;
  }

  mate::Dictionary options;
  if (!(args->Length() == 1 && args->GetNext(&options))) {
    options = mate::Dictionary::CreateEmpty(args->isolate());
  }

  return new BrowserWindow(args->isolate(), args->GetThis(), options);
}

// static
void BrowserWindow::BuildPrototype(v8::Isolate* isolate,
                                   v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "BrowserWindow"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("focusOnWebView", &BrowserWindow::FocusOnWebView)
      .SetMethod("blurWebView", &BrowserWindow::BlurWebView)
      .SetMethod("isWebViewFocused", &BrowserWindow::IsWebViewFocused)
      .SetProperty("webContents", &BrowserWindow::GetWebContents);
}

// static
v8::Local<v8::Value> BrowserWindow::From(v8::Isolate* isolate,
                                         NativeWindow* native_window) {
  auto* existing = TrackableObject::FromWrappedClass(isolate, native_window);
  if (existing)
    return existing->GetWrapper();
  else
    return v8::Null(isolate);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::BrowserWindow;
using atom::api::TopLevelWindow;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("BrowserWindow", mate::CreateConstructor<BrowserWindow>(
                                isolate, base::Bind(&BrowserWindow::New)));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_window, Initialize)
