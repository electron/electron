// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_window_mac_remote.h"

#include <algorithm>
#include <utility>

#include "base/auto_reset.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "components/remote_cocoa/browser/application_host.h"
#include "components/remote_cocoa/common/native_widget_ns_window.mojom.h"
#include "content/public/browser/desktop_media_id.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "shell/browser/api/electron_api_app_shim_host.h"
#include "shell/browser/ui/cocoa/root_view_mac.h"
#include "shell/browser/ui/views/content_view_targeter_delegate.h"
#include "shell/browser/window_list.h"
#include "shell/common/app_shim.mojom.h"
#include "shell/common/electron_paths.h"
#include "shell/common/gin_converters/gfx_converter.h"
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
                        bool has_frame,
                        bool has_title_bar_buttons)
      : views::NativeWidgetMac(delegate),
        application_host_(application_host),
        style_mask_(style_mask),
        has_frame_(has_frame),
        has_title_bar_buttons_(has_title_bar_buttons) {}

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
      // A title bar of no height, unless the window buttons are to show in
      // it (a titleBarStyle other than the default).
      if (!has_title_bar_buttons_)
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
  const bool has_title_bar_buttons_;
};

// A message box that a window in a shim is showing and that
// CloseMessageBox() can name. `serial` tells it from a later box that was
// given the same id.
struct RemoteMessageBox {
  base::WeakPtr<NativeWindowMacRemote> window;
  uint64_t serial = 0;
};

base::flat_map<int, RemoteMessageBox>& RemoteMessageBoxes() {
  static base::NoDestructor<base::flat_map<int, RemoteMessageBox>> boxes;
  return *boxes;
}

mojom::AppShimFileDialogOptionsPtr ToMojo(
    const file_dialog::DialogSettings& settings) {
  auto options = mojom::AppShimFileDialogOptions::New();
  options->title = settings.title;
  options->message = settings.message;
  options->button_label = settings.button_label;
  options->name_field_label = settings.name_field_label;
  // Resolved here: the shim's process has no path service for the default.
  options->default_path =
      settings.default_path.empty() ? GetDefaultPath() : settings.default_path;
  for (const auto& [name, extensions] : settings.filters) {
    options->filters.push_back(
        mojom::AppShimFileDialogFilter::New(name, extensions));
  }
  options->properties = settings.properties;
  options->shows_tag_field = settings.shows_tag_field;
  return options;
}

// Turns the window's close button into the window's `close` event; the window
// goes away only when that is not prevented. The shim's process is blocked on
// the answer to this, and destroying the window while it waits would drop the
// answer, so the event is emitted once the answer has gone.
class RemoteWindowClientView : public views::ClientView {
 public:
  RemoteWindowClientView(views::Widget* widget,
                         views::View* contents_view,
                         base::RepeatingClosure on_close_requested)
      : views::ClientView(widget, contents_view),
        on_close_requested_(std::move(on_close_requested)) {}

  views::CloseRequestResult OnWindowCloseRequested() override {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, on_close_requested_);
    return views::CloseRequestResult::kCannotClose;
  }

 private:
  const base::RepeatingClosure on_close_requested_;
};

// The size limits views passes to a remote window are of its content, where
// RootViewMac answers with the window's.
class RemoteRootView : public RootViewMac {
 public:
  explicit RemoteRootView(NativeWindow* window)
      : RootViewMac(window), window_(window) {}

  gfx::Size GetMinimumSize() const override {
    return window_->GetContentMinimumSize();
  }
  gfx::Size GetMaximumSize() const override {
    return window_->GetContentMaximumSize();
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
      root_view_(std::make_unique<RemoteRootView>(this)) {
  int width = options.ValueOrDefault(options::kWidth, 800);
  int height = options.ValueOrDefault(options::kHeight, 600);
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
  // Reading the options above may have run JavaScript that disconnected the
  // shim, so the host is looked up again here, with nothing left to run
  // before it is used. Without one the window is made in this process.
  api::AppShimHost* app_shim = nullptr;
  options.Get(options::kAppShim, &app_shim);
  application_host =
      app_shim ? app_shim->GetRemoteCocoaApplicationHost() : nullptr;
  auto* native_widget = new RemoteNativeWidgetMac(
      widget(), application_host, style_mask_, has_frame(),
      title_bar_style() != TitleBarStyle::kNormal);
  params.native_widget = native_widget;
  widget()->Init(std::move(params));
  native_widget->DidInit();
  if (application_host) {
    app_shim_ = app_shim;
    app_shim->DidCreateWindow();
  }

  // What remote_cocoa has no call for goes to the shim directly, on the pipe
  // that the window's creation went over, so it arrives after that.
  options.GetOptional(options::kTrafficLightPosition, &traffic_light_position_);
  if (std::string identifier;
      options.Get(options::kTabbingIdentifier, &identifier) &&
      !identifier.empty() && has_frame() && !transparent()) {
    tabbing_identifier_ = identifier;
  }
  if (std::string state; options.Get(options::kVisualEffectState, &state)) {
    visual_effect_state_ = state == "active" ? NSVisualEffectStateActive
                           : state == "inactive"
                               ? NSVisualEffectStateInactive
                               : NSVisualEffectStateFollowsWindowActiveState;
  }
  if (auto* shim = GetAppShim()) {
    auto chrome = mojom::AppShimWindowChrome::New();
    chrome->has_frame = has_frame();
    chrome->title_bar_style = static_cast<int32_t>(title_bar_style());
    chrome->traffic_light_position = traffic_light_position_;
    chrome->titlebar_overlay_height = titlebar_overlay_height();
    chrome->tabbing_identifier = tabbing_identifier_;
    shim->InitWindowChrome(GetRemoteWindowId(), std::move(chrome));
  }
  if (!movable_)
    SetMovable(false);
  widget()->SetNativeWindowProperty(kNativeWindowKey.c_str(), this);
  widget_observation_.Observe(widget());

  SetCanResize(resizable);
  SetCanMinimize(minimizable);
  SetCanMaximize(maximizable);
  widget()->OnSizeConstraintsChanged();

  // Width and height are the content's for a window.open() inner size and
  // with useContentSize.
  constexpr int kMinSizeReqdBySpec = 100;
  const int inner_width = options.ValueOrDefault(options::kinnerWidth, 0);
  const int inner_height = options.ValueOrDefault(options::kinnerHeight, 0);
  bool use_content_size =
      options.ValueOrDefault(options::kUseContentSize, false);
  if (inner_width || inner_height) {
    use_content_size = true;
    if (inner_width)
      width = std::max(kMinSizeReqdBySpec, inner_width);
    if (inner_height)
      height = std::max(kMinSizeReqdBySpec, inner_height);
  }
  if (use_content_size)
    SetContentSize(gfx::Size(width, height));

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
  NotifyWindowCloseButtonClicked();
}

void NativeWindowMacRemote::CloseImmediately() {
  if (widget_callback_depth_ == 0) {
    CloseNow();
    return;
  }
  // Called by a listener of one of the widget's own notifications, so
  // destroying the widget now would free what is notifying. The window is
  // closed as far as its observers are concerned, as they may be about to go
  // away themselves; the widget follows once the notification has returned.
  widget()->Hide();
  NotifyWindowClosed();
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&NativeWindowMacRemote::CloseNow,
                                weak_factory_.GetWeakPtr()));
}

void NativeWindowMacRemote::CloseNow() {
  if (!widget_destroyed_)
    widget()->CloseNow();
}

void NativeWindowMacRemote::OnCloseButtonClicked() {
  if (!IsClosed())
    NotifyWindowCloseButtonClicked();
}

bool NativeWindowMacRemote::IsInAppShim() const {
  return true;
}

mojom::AppShim* NativeWindowMacRemote::GetAppShim() const {
  return app_shim_ ? app_shim_->GetAppShim() : nullptr;
}

uint64_t NativeWindowMacRemote::GetRemoteWindowId() const {
  auto* host = views::NativeWidgetMacNSWindowHost::GetFromNativeWindow(
      GetNativeWindow());
  return host ? host->bridged_native_widget_id() : 0;
}

void NativeWindowMacRemote::ShowMessageBox(const MessageBoxSettings& settings,
                                           MessageBoxCallback callback) {
  // Run whatever happens: with "cancel" if the shim drops the request.
  callback = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), settings.cancel_id, false);
  auto* app_shim = GetAppShim();
  const uint64_t window_id = GetRemoteWindowId();
  if (!app_shim || !window_id)
    return;

  auto options = mojom::AppShimMessageBoxOptions::New();
  options->type = static_cast<int32_t>(settings.type);
  options->buttons = settings.buttons;
  options->default_id = settings.default_id;
  options->cancel_id = settings.cancel_id;
  options->message = settings.message;
  options->detail = settings.detail;
  options->checkbox_label = settings.checkbox_label;
  options->checkbox_checked = settings.checkbox_checked;
  options->icon = settings.icon;
  options->text_width = settings.text_width;
  options->has_id = settings.id.has_value();
  options->id = settings.id.value_or(0);
  if (settings.id) {
    // A box that is up under this id, here or in any shim, makes way, as one
    // of this process's own does.
    electron::CloseMessageBoxIfShowing(*settings.id);
    static uint64_t next_serial = 0;
    const uint64_t serial = ++next_serial;
    RemoteMessageBoxes()[*settings.id] = {weak_factory_.GetWeakPtr(), serial};
    callback = base::BindOnce(
        [](int id, uint64_t serial, MessageBoxCallback callback, int response,
           bool checked) {
          // Unless a later box has taken the id over.
          auto it = RemoteMessageBoxes().find(id);
          if (it != RemoteMessageBoxes().end() && it->second.serial == serial)
            RemoteMessageBoxes().erase(it);
          std::move(callback).Run(response, checked);
        },
        *settings.id, serial, std::move(callback));
  }
  app_shim->ShowMessageBox(window_id, std::move(options), std::move(callback));
}

int NativeWindowMacRemote::ShowMessageBoxSync(
    const MessageBoxSettings& settings) {
  // The box is in another process, so there is no modal session of this
  // one to run; tasks, and so other JavaScript, run while it is up.
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  int result = settings.cancel_id;
  ShowMessageBox(settings, base::BindOnce(
                               [](base::OnceClosure quit, int* out,
                                  int response, bool checkbox_checked) {
                                 *out = response;
                                 std::move(quit).Run();
                               },
                               run_loop.QuitClosure(), &result));
  run_loop.Run();
  return result;
}

// static
bool NativeWindowMacRemote::CloseMessageBox(int id) {
  auto it = RemoteMessageBoxes().find(id);
  if (it == RemoteMessageBoxes().end())
    return false;
  if (it->second.window) {
    if (auto* app_shim = it->second.window->GetAppShim())
      app_shim->CloseMessageBox(id);
  }
  return true;
}

void NativeWindowMacRemote::ShowOpenDialog(
    const file_dialog::DialogSettings& settings,
    file_dialog::OpenDialogCallback callback) {
  callback = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), true, std::vector<base::FilePath>());
  auto* app_shim = GetAppShim();
  const uint64_t window_id = GetRemoteWindowId();
  if (!app_shim || !window_id)
    return;
  app_shim->ShowOpenDialog(window_id, ToMojo(settings), std::move(callback));
}

void NativeWindowMacRemote::ShowSaveDialog(
    const file_dialog::DialogSettings& settings,
    file_dialog::SaveDialogCallback callback) {
  callback = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), true, base::FilePath());
  auto* app_shim = GetAppShim();
  const uint64_t window_id = GetRemoteWindowId();
  if (!app_shim || !window_id)
    return;
  app_shim->ShowSaveDialog(window_id, ToMojo(settings), std::move(callback));
}

remote_cocoa::mojom::NativeWidgetNSWindow*
NativeWindowMacRemote::GetNSWindowMojo() const {
  auto* host = views::NativeWidgetMacNSWindowHost::GetFromNativeWindow(
      GetNativeWindow());
  return host ? host->GetNSWindowMojo() : nullptr;
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
  // Widget::Restore() would also leave full screen and the Dock.
  if (auto* ns_window = GetNSWindowMojo(); ns_window && IsMaximized())
    ns_window->SetZoomed(false);
}

bool NativeWindowMacRemote::IsMaximized() const {
  return widget()->IsMaximized();
}

void NativeWindowMacRemote::Minimize() {
  widget()->Minimize();
}

void NativeWindowMacRemote::Restore() {
  // Widget::Restore() would also leave full screen and un-maximize.
  if (auto* ns_window = GetNSWindowMojo())
    ns_window->SetMiniaturized(false);
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
  if (auto* ns_window = GetNSWindowMojo())
    ns_window->SetMovable(movable);
}

bool NativeWindowMacRemote::IsMovable() const {
  return movable_;
}

void NativeWindowMacRemote::SetMinimizable(bool minimizable) {
  SetCanMinimize(minimizable);
  widget()->OnSizeConstraintsChanged();
  if (auto* ns_window = GetNSWindowMojo())
    ns_window->SetMiniaturizable(minimizable);
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
  // What views derives from the size constraints is "resizable and
  // maximizable", which is a different question.
  if (auto* ns_window = GetNSWindowMojo())
    ns_window->SetFullscreenable(fullscreenable);
}

bool NativeWindowMacRemote::IsFullScreenable() const {
  return fullscreenable_;
}

void NativeWindowMacRemote::SetClosable(bool closable) {
  closable_ = closable;
  if (auto* ns_window = GetNSWindowMojo())
    ns_window->SetClosable(closable);
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
  opacity_ = std::clamp(opacity, 0.0, 1.0);
  widget()->SetOpacity(opacity_);
}

double NativeWindowMacRemote::GetOpacity() const {
  return opacity_;
}

void NativeWindowMacRemote::SetIgnoreMouseEvents(bool ignore, bool forward) {
  if (auto* ns_window = GetNSWindowMojo())
    ns_window->SetIgnoresMouseEvents(ignore);
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

void NativeWindowMacRemote::SetWindowButtonVisibility(bool visible) {
  window_button_visibility_ = visible;
  if (auto* shim = GetAppShim())
    shim->SetWindowButtonVisibility(GetRemoteWindowId(), visible);
}

bool NativeWindowMacRemote::GetWindowButtonVisibility() const {
  // Until told otherwise they show wherever there is a title bar for them,
  // except where they wait for the pointer.
  return window_button_visibility_.value_or(
      has_frame() || title_bar_style() == TitleBarStyle::kHidden ||
      title_bar_style() == TitleBarStyle::kHiddenInset);
}

void NativeWindowMacRemote::SetWindowButtonPosition(
    std::optional<gfx::Point> position) {
  traffic_light_position_ = position;
  if (auto* shim = GetAppShim())
    shim->SetWindowButtonPosition(GetRemoteWindowId(), position);
}

std::optional<gfx::Point> NativeWindowMacRemote::GetWindowButtonPosition()
    const {
  return traffic_light_position_;
}

// The shim's process redraws them when its window changes.
void NativeWindowMacRemote::RedrawTrafficLights() {}

void NativeWindowMacRemote::SetVibrancy(const std::string& type, int duration) {
  NativeWindow::SetVibrancy(type, duration);
  if (auto* shim = GetAppShim()) {
    shim->SetWindowVibrancy(GetRemoteWindowId(), type, duration,
                            visual_effect_state_);
  }
}

void NativeWindowMacRemote::PerformTabAction(mojom::AppShimTabAction action) {
  if (auto* shim = GetAppShim())
    shim->PerformWindowTabAction(GetRemoteWindowId(), action);
}

void NativeWindowMacRemote::SelectPreviousTab() {
  PerformTabAction(mojom::AppShimTabAction::kSelectPrevious);
}

void NativeWindowMacRemote::SelectNextTab() {
  PerformTabAction(mojom::AppShimTabAction::kSelectNext);
}

void NativeWindowMacRemote::ShowAllTabs() {
  PerformTabAction(mojom::AppShimTabAction::kShowAll);
}

void NativeWindowMacRemote::MergeAllWindows() {
  PerformTabAction(mojom::AppShimTabAction::kMergeAll);
}

void NativeWindowMacRemote::MoveTabToNewWindow() {
  PerformTabAction(mojom::AppShimTabAction::kMoveToNewWindow);
}

void NativeWindowMacRemote::ToggleTabBar() {
  PerformTabAction(mojom::AppShimTabAction::kToggleTabBar);
}

bool NativeWindowMacRemote::AddTabbedWindow(NativeWindow* window) {
  // Tabs are windows of one process: the other window has to be in the same
  // shim.
  if (window == this || !window->IsInAppShim())
    return false;
  auto* other = static_cast<NativeWindowMacRemote*>(window);
  auto* shim = GetAppShim();
  if (!shim || other->GetAppShim() != shim)
    return false;
  shim->AddTabbedWindow(GetRemoteWindowId(), other->GetRemoteWindowId());
  return true;
}

std::optional<std::string> NativeWindowMacRemote::GetTabbingIdentifier() const {
  return tabbing_identifier_;
}

void NativeWindowMacRemote::PreviewFile(const std::string& path,
                                        const std::string& display_name) {
  if (auto* shim = GetAppShim()) {
    shim->PreviewFile(GetRemoteWindowId(), base::FilePath(path), display_name);
  }
}

void NativeWindowMacRemote::CloseFilePreview() {
  if (auto* shim = GetAppShim())
    shim->CloseFilePreview(GetRemoteWindowId());
}

void NativeWindowMacRemote::UpdateFrame() {}

bool NativeWindowMacRemote::IsHiddenInMissionControl() const {
  return false;
}

void NativeWindowMacRemote::SetHiddenInMissionControl(bool hidden) {}

void NativeWindowMacRemote::SetAspectRatio(double aspect_ratio,
                                           const gfx::Size& extra_size) {
  NativeWindow::SetAspectRatio(aspect_ratio, extra_size);
  // An empty ratio lifts the restriction.
  if (auto* ns_window = GetNSWindowMojo()) {
    ns_window->SetAspectRatio(
        aspect_ratio > 0.0 ? gfx::SizeF(aspect_ratio, 1.0) : gfx::SizeF(),
        extra_size);
  }
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
  return new RemoteWindowClientView(
      widget, TransferOwnershipOfContentsView(),
      base::BindRepeating(&NativeWindowMacRemote::OnCloseButtonClicked,
                          weak_factory_.GetWeakPtr()));
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
  base::AutoReset<int> in_callback(&widget_callback_depth_,
                                   widget_callback_depth_ + 1);
  if (active)
    NotifyWindowFocus();
  else
    NotifyWindowBlur();
}

void NativeWindowMacRemote::OnWidgetBoundsChanged(views::Widget* widget,
                                                  const gfx::Rect& bounds) {
  base::AutoReset<int> in_callback(&widget_callback_depth_,
                                   widget_callback_depth_ + 1);
  const gfx::Rect previous = std::exchange(last_bounds_, bounds);
  if (previous.size() != bounds.size())
    NotifyWindowResize();
  if (previous.origin() != bounds.origin() && !IsClosed()) {
    // On macOS `moved` is an alias of `move`.
    NotifyWindowMove();
    NotifyWindowMoved();
  }
}

void NativeWindowMacRemote::OnWidgetUserResizeEnded(views::Widget* widget) {
  base::AutoReset<int> in_callback(&widget_callback_depth_,
                                   widget_callback_depth_ + 1);
  NotifyWindowResized();
}

void NativeWindowMacRemote::OnWidgetVisibilityChanged(views::Widget* widget,
                                                      bool visible) {
  base::AutoReset<int> in_callback(&widget_callback_depth_,
                                   widget_callback_depth_ + 1);
  if (visible)
    NotifyWindowShow();
  else
    NotifyWindowHide();
}

void NativeWindowMacRemote::OnWidgetShowStateChanged(views::Widget* widget) {
  base::AutoReset<int> in_callback(&widget_callback_depth_,
                                   widget_callback_depth_ + 1);
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
  widget_destroyed_ = true;
  widget_observation_.Reset();
  NotifyWindowClosed();
}

}  // namespace electron
