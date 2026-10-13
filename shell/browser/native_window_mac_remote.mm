// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_window_mac_remote.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "components/remote_cocoa/browser/application_host.h"
#include "components/remote_cocoa/common/native_widget_ns_window.mojom.h"
#include "content/public/browser/desktop_media_id.h"
#include "shell/browser/api/electron_api_app_shim_host.h"
#include "shell/browser/ui/cocoa/root_view_mac.h"
#include "shell/browser/ui/views/content_view_targeter_delegate.h"
#include "shell/browser/window_list.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/options_switches.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/background.h"
#include "ui/views/cocoa/native_widget_mac_ns_window_host.h"
#include "ui/views/view_targeter.h"
#include "ui/views/widget/native_widget_mac.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/client_view.h"
#include "ui/views/window/native_frame_view_mac.h"

namespace electron {

namespace {

// Makes the widget's NSWindow in the app shim's process.
class RemoteNativeWidgetMac : public views::NativeWidgetMac {
 public:
  RemoteNativeWidgetMac(views::internal::NativeWidgetDelegate* delegate,
                        remote_cocoa::ApplicationHost* application_host,
                        NSUInteger style_mask,
                        bool has_frame)
      : views::NativeWidgetMac(delegate),
        application_host_(application_host),
        style_mask_(style_mask),
        has_frame_(has_frame) {}

  RemoteNativeWidgetMac(const RemoteNativeWidgetMac&) = delete;
  RemoteNativeWidgetMac& operator=(const RemoteNativeWidgetMac&) = delete;

  // The host is only read while the widget is initialized; after that the
  // widget's own window host observes it.
  void DidInit() { application_host_ = nullptr; }

 protected:
  // views::NativeWidgetMac:
  void PopulateCreateWindowParams(
      const views::Widget::InitParams& widget_params,
      remote_cocoa::mojom::CreateWindowParams* params) override {
    params->style_mask = style_mask_;
    if (!has_frame_) {
      params->window_class = remote_cocoa::mojom::WindowClass::kFrameless;
      params->titlebar_appears_transparent = true;
      params->window_title_hidden = true;
    }
  }

  remote_cocoa::ApplicationHost* GetRemoteCocoaApplicationHost() override {
    return application_host_;
  }

 private:
  raw_ptr<remote_cocoa::ApplicationHost> application_host_;
  const NSUInteger style_mask_;
  const bool has_frame_;
};

// Turns the window's close button into the window's `close` event; the window
// goes away only when that is not prevented.
class RemoteWindowClientView : public views::ClientView {
 public:
  RemoteWindowClientView(views::Widget* widget,
                         views::View* contents_view,
                         NativeWindow* window)
      : views::ClientView(widget, contents_view), window_(window) {}

  views::CloseRequestResult OnWindowCloseRequested() override {
    window_->NotifyWindowCloseButtonClicked();
    return views::CloseRequestResult::kCannotClose;
  }

 private:
  const raw_ptr<NativeWindow> window_;
};

// views' frame view for macOS asks the NSWindow how much taller the window is
// than its content. The NSWindow in this process is a stand-in without a title
// bar, so the answer comes from the real window's style instead.
class RemoteFrameView : public views::NativeFrameViewMac {
 public:
  RemoteFrameView(views::Widget* widget, const NativeWindowMacRemote* window)
      : views::NativeFrameViewMac(widget), window_(window) {}

  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override {
    gfx::Rect window_bounds =
        window_->ContentBoundsToWindowBounds(client_bounds);
    if (window_bounds.IsEmpty())
      window_bounds.set_size(gfx::Size(1, 1));
    return window_bounds;
  }

 private:
  const raw_ptr<const NativeWindowMacRemote> window_;
};

}  // namespace

// static
remote_cocoa::ApplicationHost*
NativeWindowMacRemote::ApplicationHostFromOptions(
    const gin_helper::Dictionary& options) {
  api::AppShimHost* app_shim = nullptr;
  if (!options.Get(options::kAppShim, &app_shim) || !app_shim)
    return nullptr;
  return app_shim->GetRemoteCocoaApplicationHost();
}

NativeWindowMacRemote::NativeWindowMacRemote(
    const int32_t base_window_id,
    const gin_helper::Dictionary& options,
    NativeWindow* parent,
    remote_cocoa::ApplicationHost* application_host)
    : NativeWindow{base_window_id, options, parent},
      root_view_(std::make_unique<RootViewMac>(this)) {
  const int width = options.ValueOrDefault(options::kWidth, 800);
  const int height = options.ValueOrDefault(options::kHeight, 600);
  const NSRect main_screen_rect = NSScreen.screens.firstObject.frame;
  const gfx::Rect bounds(round((NSWidth(main_screen_rect) - width) / 2),
                         round((NSHeight(main_screen_rect) - height) / 2),
                         width, height);

  const bool resizable = options.ValueOrDefault(options::kResizable, true);
  const bool minimizable = options.ValueOrDefault(options::kMinimizable, true);
  const bool maximizable = options.ValueOrDefault(options::kMaximizable, true);
  closable_ = options.ValueOrDefault(options::kClosable, true);
  movable_ = options.ValueOrDefault(options::kMovable, true);

  // A frameless window keeps a (zero height) title bar, as it does in this
  // process, so that it has the system's corners, shadow and resize edges.
  style_mask_ = NSWindowStyleMaskTitled;
  if (minimizable)
    style_mask_ |= NSWindowStyleMaskMiniaturizable;
  if (closable_)
    style_mask_ |= NSWindowStyleMaskClosable;
  if (resizable)
    style_mask_ |= NSWindowStyleMaskResizable;
  if (!has_frame())
    style_mask_ |= NSWindowStyleMaskFullSizeContentView;

  views::Widget::InitParams params(
      views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW);
  params.bounds = bounds;
  params.delegate = this;
  if (IsTranslucent())
    params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  auto* native_widget = new RemoteNativeWidgetMac(widget(), application_host,
                                                  style_mask_, has_frame());
  params.native_widget = native_widget;
  widget()->Init(std::move(params));
  native_widget->DidInit();
  widget()->SetNativeWindowProperty(kNativeWindowKey.c_str(), this);
  widget_observation_.Observe(widget());

  SetCanResize(resizable);
  SetCanMinimize(minimizable);
  SetCanMaximize(maximizable);
  widget()->OnSizeConstraintsChanged();

  last_bounds_ = widget()->GetWindowBoundsInScreen();
}

NativeWindowMacRemote::~NativeWindowMacRemote() = default;

void NativeWindowMacRemote::OnTitleChanged() {
  widget()->UpdateWindowTitle();
}

void NativeWindowMacRemote::SetContentView(views::View* view) {
  views::View* root_view = GetContentsView();
  if (views::View* old_view = content_view()) {
    set_content_view(nullptr);
    if (old_view->owned_by_client())
      root_view->RemoveChildView(old_view);
    else
      root_view->RemoveChildViewT(old_view);
  }

  set_content_view(view);

  // Events fall through the content view to a sibling WebContentsView.
  view->SetEventTargeter(std::make_unique<views::ViewTargeter>(
      std::make_unique<ContentViewTargeterDelegate>()));

  root_view->AddChildViewRaw(content_view());

  FlushPendingRootLayout(root_view);
}

void NativeWindowMacRemote::Close() {
  if (!IsClosable()) {
    WindowList::WindowCloseCancelled(this);
    return;
  }
  // Asks RemoteWindowClientView, which emits `close`.
  widget()->Close();
}

void NativeWindowMacRemote::CloseImmediately() {
  widget()->CloseNow();
}

void NativeWindowMacRemote::Focus(bool focus) {
  if (!IsVisible())
    return;
  if (focus)
    widget()->Activate();
  else
    widget()->Deactivate();
}

bool NativeWindowMacRemote::IsFocused() const {
  return widget()->IsActive();
}

void NativeWindowMacRemote::Show() {
  FlushPendingDisplayMode();
  widget()->Show();
}

void NativeWindowMacRemote::ShowInactive() {
  FlushPendingDisplayMode();
  widget()->ShowInactive();
}

void NativeWindowMacRemote::Hide() {
  widget()->Hide();
}

bool NativeWindowMacRemote::IsVisible() const {
  return widget()->IsVisible() && !IsMinimized();
}

bool NativeWindowMacRemote::IsEnabled() const {
  return true;
}

void NativeWindowMacRemote::SetEnabled(bool enable) {}

void NativeWindowMacRemote::Maximize() {
  if (!IsVisible())
    widget()->Show();
  widget()->Maximize();
}

void NativeWindowMacRemote::Unmaximize() {
  if (IsMaximized())
    widget()->Restore();
}

bool NativeWindowMacRemote::IsMaximized() const {
  return widget()->IsMaximized();
}

void NativeWindowMacRemote::Minimize() {
  widget()->Minimize();
}

void NativeWindowMacRemote::Restore() {
  widget()->Restore();
}

bool NativeWindowMacRemote::IsMinimized() const {
  return widget()->IsMinimized();
}

void NativeWindowMacRemote::SetFullScreen(bool fullscreen) {
  if (fullscreen == IsFullscreen() || (fullscreen && !IsFullScreenable()))
    return;
  widget()->SetFullscreen(fullscreen);
}

bool NativeWindowMacRemote::IsFullscreen() const {
  return widget()->IsFullscreen();
}

void NativeWindowMacRemote::SetBounds(const gfx::Rect& bounds, bool animate) {
  widget()->SetBounds(bounds);
}

gfx::Rect NativeWindowMacRemote::GetBounds() const {
  return widget()->GetWindowBoundsInScreen();
}

gfx::Rect NativeWindowMacRemote::GetNormalBounds() const {
  return widget()->GetRestoredBounds();
}

void NativeWindowMacRemote::SetSizeConstraints(
    const extensions::SizeConstraints& window_constraints) {
  NativeWindow::SetSizeConstraints(window_constraints);
  widget()->OnSizeConstraintsChanged();
}

void NativeWindowMacRemote::SetContentSizeConstraints(
    const extensions::SizeConstraints& size_constraints) {
  NativeWindow::SetContentSizeConstraints(size_constraints);
  widget()->OnSizeConstraintsChanged();
}

void NativeWindowMacRemote::SetResizable(bool resizable) {
  SetCanResize(resizable);
  widget()->OnSizeConstraintsChanged();
}

bool NativeWindowMacRemote::MoveAbove(const std::string& sourceId) {
  return false;
}

void NativeWindowMacRemote::MoveTop() {
  widget()->StackAtTop();
}

bool NativeWindowMacRemote::IsResizable() const {
  return CanResize();
}

void NativeWindowMacRemote::SetMovable(bool movable) {
  movable_ = movable;
}

bool NativeWindowMacRemote::IsMovable() const {
  return movable_;
}

void NativeWindowMacRemote::SetMinimizable(bool minimizable) {
  SetCanMinimize(minimizable);
  widget()->OnSizeConstraintsChanged();
}

bool NativeWindowMacRemote::IsMinimizable() const {
  return CanMinimize();
}

void NativeWindowMacRemote::SetMaximizable(bool maximizable) {
  SetCanMaximize(maximizable);
  widget()->OnSizeConstraintsChanged();
}

bool NativeWindowMacRemote::IsMaximizable() const {
  return CanMaximize();
}

void NativeWindowMacRemote::SetFullScreenable(bool fullscreenable) {
  fullscreenable_ = fullscreenable;
  SetCanFullscreen(fullscreenable);
  widget()->OnSizeConstraintsChanged();
}

bool NativeWindowMacRemote::IsFullScreenable() const {
  return fullscreenable_;
}

void NativeWindowMacRemote::SetClosable(bool closable) {
  closable_ = closable;
}

bool NativeWindowMacRemote::IsClosable() const {
  return closable_;
}

void NativeWindowMacRemote::SetAlwaysOnTop(ui::ZOrderLevel z_order,
                                           const std::string& level,
                                           int relative_level) {
  const bool was_on_top = GetZOrderLevel() != ui::ZOrderLevel::kNormal;
  widget()->SetZOrderLevel(z_order);
  always_on_top_level_ = z_order == ui::ZOrderLevel::kNormal ? "" : level;
  const bool is_on_top = z_order != ui::ZOrderLevel::kNormal;
  if (was_on_top != is_on_top)
    NativeWindow::NotifyWindowAlwaysOnTopChanged(is_on_top);
}

std::string NativeWindowMacRemote::GetAlwaysOnTopLevel() const {
  return always_on_top_level_;
}

ui::ZOrderLevel NativeWindowMacRemote::GetZOrderLevel() const {
  return widget()->GetZOrderLevel();
}

void NativeWindowMacRemote::Center() {
  widget()->CenterWindow(GetSize());
}

void NativeWindowMacRemote::Invalidate() {
  root_view_->SchedulePaint();
}

bool NativeWindowMacRemote::IsActive() const {
  return widget()->IsActive();
}

void NativeWindowMacRemote::SetActive(bool is_key) {}

void NativeWindowMacRemote::RemoveChildFromParentWindow() {}

void NativeWindowMacRemote::RemoveChildWindow(NativeWindow* child) {}

void NativeWindowMacRemote::AttachChildren() {}

void NativeWindowMacRemote::DetachChildren() {}

void NativeWindowMacRemote::FlashFrame(bool flash) {}

void NativeWindowMacRemote::SetSkipTaskbar(bool skip) {}

void NativeWindowMacRemote::SetExcludedFromShownWindowsMenu(bool excluded) {
  excluded_from_windows_menu_ = excluded;
}

bool NativeWindowMacRemote::IsExcludedFromShownWindowsMenu() const {
  return excluded_from_windows_menu_;
}

void NativeWindowMacRemote::SetSimpleFullScreen(bool simple_fullscreen) {}

bool NativeWindowMacRemote::IsSimpleFullScreen() const {
  return false;
}

void NativeWindowMacRemote::SetKiosk(bool kiosk) {
  if (kiosk == is_kiosk_)
    return;
  is_kiosk_ = kiosk;
  SetFullScreen(kiosk);
}

bool NativeWindowMacRemote::IsKiosk() const {
  return is_kiosk_;
}

void NativeWindowMacRemote::SetBackgroundColor(SkColor color) {
  background_color_ = color;
  root_view_->SetBackground(
      transparent() ? nullptr : views::CreateSolidBackground(color));
  root_view_->SchedulePaint();
}

SkColor NativeWindowMacRemote::GetBackgroundColor() const {
  return background_color_;
}

void NativeWindowMacRemote::SetHasShadow(bool has_shadow) {
  has_shadow_ = has_shadow;
}

bool NativeWindowMacRemote::HasShadow() const {
  return has_shadow_;
}

void NativeWindowMacRemote::SetOpacity(const double opacity) {
  opacity_ = ClampOpacity(opacity);
  widget()->SetOpacity(opacity_);
}

double NativeWindowMacRemote::GetOpacity() const {
  return opacity_;
}

void NativeWindowMacRemote::SetIgnoreMouseEvents(bool ignore, bool forward) {
  auto* host = views::NativeWidgetMacNSWindowHost::GetFromNativeWindow(
      GetNativeWindow());
  if (host && host->GetNSWindowMojo())
    host->GetNSWindowMojo()->SetIgnoresMouseEvents(ignore);
}

void NativeWindowMacRemote::SetContentProtection(bool enable) {
  content_protected_ = enable;
  widget()->SetAllowScreenshots(!enable);
}

bool NativeWindowMacRemote::IsContentProtected() const {
  return content_protected_;
}

content::DesktopMediaID NativeWindowMacRemote::GetDesktopMediaID() const {
  return content::DesktopMediaID(content::DesktopMediaID::TYPE_WINDOW,
                                 GetAcceleratedWidget());
}

gfx::NativeView NativeWindowMacRemote::GetNativeView() const {
  return widget()->GetNativeView();
}

gfx::NativeWindow NativeWindowMacRemote::GetNativeWindow() const {
  return widget()->GetNativeWindow();
}

gfx::AcceleratedWidget NativeWindowMacRemote::GetAcceleratedWidget() const {
  // The stand-in window of this process, as there is no way to name another
  // process's window with this type.
  return GetNativeWindow().GetNativeNSWindow().windowNumber;
}

NativeWindowHandle NativeWindowMacRemote::GetNativeWindowHandle() const {
  return GetNativeView();
}

void NativeWindowMacRemote::SetProgressBar(double progress,
                                           const ProgressState state) {}

void NativeWindowMacRemote::SetOverlayIcon(const gfx::Image& overlay,
                                           const std::string& description) {}

void NativeWindowMacRemote::SetVisibleOnAllWorkspaces(
    bool visible,
    bool visibleOnFullScreen,
    bool skipTransformProcessType) {
  widget()->SetVisibleOnAllWorkspaces(visible);
}

bool NativeWindowMacRemote::IsVisibleOnAllWorkspaces() const {
  return widget()->IsVisibleOnAllWorkspaces();
}

void NativeWindowMacRemote::SetWindowButtonVisibility(bool visible) {}

bool NativeWindowMacRemote::GetWindowButtonVisibility() const {
  return has_frame();
}

void NativeWindowMacRemote::SetWindowButtonPosition(
    std::optional<gfx::Point> position) {}

std::optional<gfx::Point> NativeWindowMacRemote::GetWindowButtonPosition()
    const {
  return std::nullopt;
}

void NativeWindowMacRemote::RedrawTrafficLights() {}

void NativeWindowMacRemote::UpdateFrame() {}

bool NativeWindowMacRemote::IsHiddenInMissionControl() const {
  return false;
}

void NativeWindowMacRemote::SetHiddenInMissionControl(bool hidden) {}

void NativeWindowMacRemote::SetAspectRatio(double aspect_ratio,
                                           const gfx::Size& extra_size) {
  NativeWindow::SetAspectRatio(aspect_ratio, extra_size);
  if (aspect_ratio > 0.0)
    widget()->SetAspectRatio(gfx::SizeF(aspect_ratio, 1.0));
}

int NativeWindowMacRemote::TitleBarHeight() const {
  if (!has_frame())
    return 0;
  const NSRect content = NSMakeRect(0, 0, 100, 100);
  const NSRect frame = [NSWindow frameRectForContentRect:content
                                               styleMask:style_mask_];
  return NSHeight(frame) - NSHeight(content);
}

gfx::Rect NativeWindowMacRemote::ContentBoundsToWindowBounds(
    const gfx::Rect& bounds) const {
  gfx::Rect window_bounds(bounds);
  const int title_bar_height = TitleBarHeight();
  window_bounds.set_y(window_bounds.y() - title_bar_height);
  window_bounds.set_height(window_bounds.height() + title_bar_height);
  return window_bounds;
}

gfx::Rect NativeWindowMacRemote::WindowBoundsToContentBounds(
    const gfx::Rect& bounds) const {
  gfx::Rect content_bounds(bounds);
  const int title_bar_height = TitleBarHeight();
  content_bounds.set_y(content_bounds.y() + title_bar_height);
  content_bounds.set_height(
      std::max(0, content_bounds.height() - title_bar_height));
  return content_bounds;
}

views::View* NativeWindowMacRemote::GetContentsView() {
  return root_view_.get();
}

views::ClientView* NativeWindowMacRemote::CreateClientView(
    views::Widget* widget) {
  return new RemoteWindowClientView(widget, TransferOwnershipOfContentsView(),
                                    this);
}

std::unique_ptr<views::FrameView> NativeWindowMacRemote::CreateFrameView(
    views::Widget* widget) {
  auto frame_view = std::make_unique<RemoteFrameView>(widget, this);
  // The shim asks, over Mojo, whether a point of the page drags the window.
  frame_view->set_non_client_hit_test_callback(
      base::BindRepeating(&NativeWindowMacRemote::FrameViewNonClientHitTest,
                          base::Unretained(this)));
  return frame_view;
}

int NativeWindowMacRemote::FrameViewNonClientHitTest(const gfx::Point& point) {
  if (widget()->IsFullscreen())
    return HTCLIENT;
  // A draggable region of a frameless window's page.
  const int contents_hit_test = NonClientHitTest(point);
  return contents_hit_test != HTNOWHERE ? contents_hit_test : HTCLIENT;
}

void NativeWindowMacRemote::OnWidgetActivationChanged(views::Widget* widget,
                                                      bool active) {
  if (active)
    NotifyWindowFocus();
  else
    NotifyWindowBlur();
}

void NativeWindowMacRemote::OnWidgetBoundsChanged(views::Widget* widget,
                                                  const gfx::Rect& bounds) {
  const gfx::Rect previous = std::exchange(last_bounds_, bounds);
  if (previous.size() != bounds.size()) {
    NotifyWindowResize();
    NotifyWindowResized();
  }
  if (previous.origin() != bounds.origin()) {
    NotifyWindowMove();
    NotifyWindowMoved();
  }
}

void NativeWindowMacRemote::OnWidgetVisibilityChanged(views::Widget* widget,
                                                      bool visible) {
  if (visible)
    NotifyWindowShow();
  else
    NotifyWindowHide();
}

void NativeWindowMacRemote::OnWidgetShowStateChanged(views::Widget* widget) {
  const bool minimized = widget->IsMinimized();
  if (std::exchange(was_minimized_, minimized) != minimized) {
    if (minimized)
      NotifyWindowMinimize();
    else
      NotifyWindowRestore();
  }
  const bool fullscreen = widget->IsFullscreen();
  if (std::exchange(was_fullscreen_, fullscreen) != fullscreen) {
    if (fullscreen)
      NotifyWindowEnterFullScreen();
    else
      NotifyWindowLeaveFullScreen();
  }
  const bool maximized = widget->IsMaximized();
  if (std::exchange(was_maximized_, maximized) != maximized) {
    if (maximized)
      NotifyWindowMaximize();
    else
      NotifyWindowUnmaximize();
  }
}

void NativeWindowMacRemote::OnWidgetDestroyed(views::Widget* widget) {
  widget_observation_.Reset();
  NotifyWindowClosed();
}

}  // namespace electron
