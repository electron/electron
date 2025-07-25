// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_window.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/web_contents_user_data.h"
#include "include/core/SkColor.h"
#include "shell/browser/background_throttling_source.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/draggable_region_provider.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/native_window_features.h"
#include "shell/browser/ui/drag_util.h"
#include "shell/browser/window_list.h"
#include "shell/common/color_util.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/persistent_dictionary.h"
#include "shell/common/options_switches.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/compositor.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"
#include "ui/views/widget/widget.h"

#if !BUILDFLAG(IS_MAC)
#include "shell/browser/ui/views/frameless_view.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "ui/display/win/screen_win.h"
#include "ui/views/views_features.h"
#endif

#if defined(USE_OZONE)
#include "ui/base/ui_base_features.h"
#include "ui/ozone/public/ozone_platform.h"
#endif

namespace gin {

template <>
struct Converter<electron::NativeWindow::TitleBarStyle> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::NativeWindow::TitleBarStyle* out) {
    using TitleBarStyle = electron::NativeWindow::TitleBarStyle;
    std::string title_bar_style;
    if (!ConvertFromV8(isolate, val, &title_bar_style))
      return false;
    if (title_bar_style == "hidden") {
      *out = TitleBarStyle::kHidden;
#if BUILDFLAG(IS_MAC)
    } else if (title_bar_style == "hiddenInset") {
      *out = TitleBarStyle::kHiddenInset;
    } else if (title_bar_style == "customButtonsOnHover") {
      *out = TitleBarStyle::kCustomButtonsOnHover;
#endif
    } else {
      return false;
    }
    return true;
  }
};

}  // namespace gin

namespace electron {

namespace {

#if BUILDFLAG(IS_WIN)
gfx::Size GetExpandedWindowSize(const NativeWindow* window,
                                bool transparent,
                                gfx::Size size) {
  if (!base::FeatureList::IsEnabled(
          views::features::kEnableTransparentHwndEnlargement) ||
      !transparent) {
    return size;
  }

  gfx::Size min_size = display::win::GetScreenWin()->ScreenToDIPSize(
      window->GetAcceleratedWidget(), gfx::Size{64, 64});

  // Some AMD drivers can't display windows that are less than 64x64 pixels,
  // so expand them to be at least that size. http://crbug.com/286609
  gfx::Size expanded(std::max(size.width(), min_size.width()),
                     std::max(size.height(), min_size.height()));
  return expanded;
}
#endif

}  // namespace

NativeWindow::NativeWindow(const gin_helper::Dictionary& options,
                           NativeWindow* parent)
    : title_bar_style_{options.ValueOrDefault(options::kTitleBarStyle,
                                              TitleBarStyle::kNormal)},
      transparent_{options.ValueOrDefault(options::kTransparent, false)},
      enable_larger_than_screen_{
          options.ValueOrDefault(options::kEnableLargerThanScreen, false)},
      is_modal_{parent != nullptr && options.ValueOrDefault("modal", false)},
      has_frame_{options.ValueOrDefault(options::kFrame, true) &&
                 title_bar_style_ == TitleBarStyle::kNormal},
      parent_{parent} {
#if BUILDFLAG(IS_WIN)
  options.Get(options::kBackgroundMaterial, &background_material_);
#elif BUILDFLAG(IS_MAC)
  options.Get(options::kVibrancyType, &vibrancy_);
#endif

  options.Get(options::kName, &window_name_);

  if (gin_helper::Dictionary persistence_options;
      options.Get(options::kWindowStatePersistence, &persistence_options)) {
    // Restore bounds by default
    restore_bounds_ = true;
    persistence_options.Get(options::kBounds, &restore_bounds_);
    // Restore display mode by default
    restore_display_mode_ = true;
    persistence_options.Get(options::kDisplayMode, &restore_display_mode_);
    window_state_persistence_enabled_ = true;
  } else if (bool flag; options.Get(options::kWindowStatePersistence, &flag)) {
    restore_bounds_ = flag;
    restore_display_mode_ = flag;
    window_state_persistence_enabled_ = flag;
  }

  // Initialize prefs_ to save/restore window bounds if we have a valid window
  // name and window state persistence is enabled.
  if (window_state_persistence_enabled_ && !window_name_.empty()) {
    if (auto* browser_process =
            electron::ElectronBrowserMainParts::Get()->browser_process()) {
      DCHECK(browser_process);
      prefs_ = browser_process->local_state();
    }
  } else if (window_state_persistence_enabled_ && window_name_.empty()) {
    LOG(WARNING) << "Window state persistence enabled but no window name "
                    "provided. Window state will not be persisted.";
  }

  if (gin_helper::Dictionary dict;
      options.Get(options::ktitleBarOverlay, &dict)) {
    titlebar_overlay_ = true;
    titlebar_overlay_height_ = dict.ValueOrDefault(options::kOverlayHeight, 0);
  } else if (bool flag; options.Get(options::ktitleBarOverlay, &flag)) {
    titlebar_overlay_ = flag;
  }

  WindowList::AddWindow(this);
}

NativeWindow::~NativeWindow() {
  // It's possible that the windows gets destroyed before it's closed, in that
  // case we need to ensure the Widget delegate gets destroyed and
  // OnWindowClosed message is still notified.
  if (widget_->widget_delegate())
    widget_->OnNativeWidgetDestroyed();
  NotifyWindowClosed();
}

void NativeWindow::InitFromOptions(const gin_helper::Dictionary& options) {
  // Setup window from options.
  if (int x, y; options.Get(options::kX, &x) && options.Get(options::kY, &y)) {
    SetPosition(gfx::Point{x, y});

#if BUILDFLAG(IS_WIN)
    // FIXME(felixrieseberg): Dirty, dirty workaround for
    // https://github.com/electron/electron/issues/10862
    // Somehow, we need to call `SetBounds` twice to get
    // usable results. The root cause is still unknown.
    SetPosition(gfx::Point{x, y});
#endif
  } else if (bool center; options.Get(options::kCenter, &center) && center) {
    Center();
  }

  const bool use_content_size =
      options.ValueOrDefault(options::kUseContentSize, false);

  // On Linux and Window we may already have maximum size defined.
  extensions::SizeConstraints size_constraints(
      use_content_size ? GetContentSizeConstraints() : GetSizeConstraints());

  const int min_width = options.ValueOrDefault(
      options::kMinWidth, size_constraints.GetMinimumSize().width());
  const int min_height = options.ValueOrDefault(
      options::kMinHeight, size_constraints.GetMinimumSize().height());
  size_constraints.set_minimum_size(gfx::Size(min_width, min_height));

  gfx::Size max_size = size_constraints.GetMaximumSize();
  int max_width = max_size.width() > 0 ? max_size.width() : INT_MAX;
  int max_height = max_size.height() > 0 ? max_size.height() : INT_MAX;
  bool have_max_width = options.Get(options::kMaxWidth, &max_width);
  if (have_max_width && max_width <= 0)
    max_width = INT_MAX;
  bool have_max_height = options.Get(options::kMaxHeight, &max_height);
  if (have_max_height && max_height <= 0)
    max_height = INT_MAX;

  // By default the window has a default maximum size that prevents it
  // from being resized larger than the screen, so we should only set this
  // if the user has passed in values.
  if (have_max_height || have_max_width || !max_size.IsEmpty())
    size_constraints.set_maximum_size(gfx::Size(max_width, max_height));

  if (use_content_size) {
    SetContentSizeConstraints(size_constraints);
  } else {
    SetSizeConstraints(size_constraints);
  }
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
  if (bool val; options.Get(options::kClosable, &val))
    SetClosable(val);
#endif

  if (bool val; options.Get(options::kMovable, &val))
    SetMovable(val);

  if (bool val; options.Get(options::kHasShadow, &val))
    SetHasShadow(val);

  if (double val; options.Get(options::kOpacity, &val))
    SetOpacity(val);

  if (bool val; options.Get(options::kAlwaysOnTop, &val) && val)
    SetAlwaysOnTop(ui::ZOrderLevel::kFloatingWindow);

  bool fullscreenable = true;
  bool fullscreen = false;
  if (options.Get(options::kFullscreen, &fullscreen) && !fullscreen) {
    // Disable fullscreen button if 'fullscreen' is specified to false.
#if BUILDFLAG(IS_MAC)
    fullscreenable = false;
#endif
  }

  options.Get(options::kFullScreenable, &fullscreenable);
  SetFullScreenable(fullscreenable);

  // Restore window state (bounds and display mode) at this point in
  // initialization. We deliberately restore bounds before display modes
  // (fullscreen/kiosk) since the target display for these states depends on the
  // window's initial bounds. Also, restoring here ensures we respect min/max
  // width/height and fullscreenable constraints.
  if (prefs_ && !window_name_.empty())
    RestoreWindowState(options);

  if (fullscreen && !restore_display_mode_)
    SetFullScreen(true);

  if (bool val; options.Get(options::kResizable, &val))
    SetResizable(val);

  if (bool val; options.Get(options::kSkipTaskbar, &val))
    SetSkipTaskbar(val);

  if (bool val;
      options.Get(options::kKiosk, &val) && val && !restore_display_mode_)
    SetKiosk(val);

#if BUILDFLAG(IS_MAC)
  if (std::string val; options.Get(options::kVibrancyType, &val))
    SetVibrancy(val, 0);
#elif BUILDFLAG(IS_WIN)
  if (std::string val; options.Get(options::kBackgroundMaterial, &val))
    SetBackgroundMaterial(val);
#endif

  SkColor background_color = SK_ColorWHITE;
  if (std::string color; options.Get(options::kBackgroundColor, &color)) {
    background_color = ParseCSSColor(color).value_or(SK_ColorWHITE);
  } else if (IsTranslucent()) {
    background_color = SK_ColorTRANSPARENT;
  }
  SetBackgroundColor(background_color);

  SetTitle(options.ValueOrDefault(options::kTitle, Browser::Get()->GetName()));
  // Save updated window state after restoration adjustments are complete if
  // any.
  SaveWindowState();
  // Then show it.
  if (options.ValueOrDefault(options::kShow, true))
    Show();
}

// static
NativeWindow* NativeWindow::FromWidget(const views::Widget* widget) {
  DCHECK(widget);
  return static_cast<NativeWindow*>(
      widget->GetNativeWindowProperty(kNativeWindowKey.c_str()));
}

void NativeWindow::SetShape(const std::vector<gfx::Rect>& rects) {
  widget()->SetShape(std::make_unique<std::vector<gfx::Rect>>(rects));
}

bool NativeWindow::IsClosed() const {
  return is_closed_;
}

void NativeWindow::SetSize(const gfx::Size& size, bool animate) {
  SetBounds(gfx::Rect(GetPosition(), size), animate);
}

gfx::Size NativeWindow::GetSize() const {
  return GetBounds().size();
}

void NativeWindow::SetPosition(const gfx::Point& position, bool animate) {
  SetBounds(gfx::Rect(position, GetSize()), animate);
}

gfx::Point NativeWindow::GetPosition() const {
  return GetBounds().origin();
}

void NativeWindow::SetContentSize(const gfx::Size& size, bool animate) {
  SetSize(ContentBoundsToWindowBounds(gfx::Rect(size)).size(), animate);
}

gfx::Size NativeWindow::GetContentSize() const {
  return GetContentBounds().size();
}

void NativeWindow::SetContentBounds(const gfx::Rect& bounds, bool animate) {
  SetBounds(ContentBoundsToWindowBounds(bounds), animate);
}

gfx::Rect NativeWindow::GetContentBounds() const {
  return WindowBoundsToContentBounds(GetBounds());
}

bool NativeWindow::IsNormal() const {
  return !IsMinimized() && !IsMaximized() && !IsFullscreen();
}

void NativeWindow::SetSizeConstraints(
    const extensions::SizeConstraints& window_constraints) {
  size_constraints_ = window_constraints;
  content_size_constraints_.reset();
}

extensions::SizeConstraints NativeWindow::GetSizeConstraints() const {
  if (size_constraints_)
    return *size_constraints_;
  if (!content_size_constraints_)
    return {};
  // Convert content size constraints to window size constraints.
  extensions::SizeConstraints constraints;
  if (content_size_constraints_->HasMaximumSize()) {
    gfx::Rect max_bounds = ContentBoundsToWindowBounds(
        gfx::Rect(content_size_constraints_->GetMaximumSize()));
    constraints.set_maximum_size(max_bounds.size());
  }
  if (content_size_constraints_->HasMinimumSize()) {
    gfx::Rect min_bounds = ContentBoundsToWindowBounds(
        gfx::Rect(content_size_constraints_->GetMinimumSize()));
    constraints.set_minimum_size(min_bounds.size());
  }
  return constraints;
}

void NativeWindow::SetContentSizeConstraints(
    const extensions::SizeConstraints& size_constraints) {
  content_size_constraints_ = size_constraints;
  size_constraints_.reset();
}

// Windows/Linux:
// The return value of GetContentSizeConstraints will be passed to Chromium
// to set min/max sizes of window. Note that we are returning content size
// instead of window size because that is what Chromium expects, see the
// comment of |WidgetSizeIsClientSize| in Chromium's codebase to learn more.
//
// macOS:
// The min/max sizes are set directly by calling NSWindow's methods.
extensions::SizeConstraints NativeWindow::GetContentSizeConstraints() const {
  if (content_size_constraints_)
    return *content_size_constraints_;
  if (!size_constraints_)
    return {};
  // Convert window size constraints to content size constraints.
  // Note that we are not caching the results, because Chromium reccalculates
  // window frame size everytime when min/max sizes are passed, and we must
  // do the same otherwise the resulting size with frame included will be wrong.
  extensions::SizeConstraints constraints;
  if (size_constraints_->HasMaximumSize()) {
    gfx::Rect max_bounds = WindowBoundsToContentBounds(
        gfx::Rect(size_constraints_->GetMaximumSize()));
    constraints.set_maximum_size(max_bounds.size());
  }
  if (size_constraints_->HasMinimumSize()) {
    gfx::Rect min_bounds = WindowBoundsToContentBounds(
        gfx::Rect(size_constraints_->GetMinimumSize()));
    constraints.set_minimum_size(min_bounds.size());
  }
  return constraints;
}

void NativeWindow::SetMinimumSize(const gfx::Size& size) {
  extensions::SizeConstraints size_constraints = GetSizeConstraints();
  size_constraints.set_minimum_size(size);
  SetSizeConstraints(size_constraints);
}

gfx::Size NativeWindow::GetMinimumSize() const {
  return GetSizeConstraints().GetMinimumSize();
}

void NativeWindow::SetMaximumSize(const gfx::Size& size) {
  extensions::SizeConstraints size_constraints = GetSizeConstraints();
  size_constraints.set_maximum_size(size);
  SetSizeConstraints(size_constraints);
}

gfx::Size NativeWindow::GetMaximumSize() const {
  return GetSizeConstraints().GetMaximumSize();
}

gfx::Size NativeWindow::GetContentMinimumSize() const {
  return GetContentSizeConstraints().GetMinimumSize();
}

gfx::Size NativeWindow::GetContentMaximumSize() const {
  const auto size_constraints = GetContentSizeConstraints();
  gfx::Size maximum_size = size_constraints.GetMaximumSize();

#if BUILDFLAG(IS_WIN)
  if (size_constraints.HasMaximumSize())
    maximum_size = GetExpandedWindowSize(this, transparent(), maximum_size);
#endif

  return maximum_size;
}

void NativeWindow::SetSheetOffset(const double offsetX, const double offsetY) {
  sheet_offset_x_ = offsetX;
  sheet_offset_y_ = offsetY;
}

double NativeWindow::GetSheetOffsetX() const {
  return sheet_offset_x_;
}

double NativeWindow::GetSheetOffsetY() const {
  return sheet_offset_y_;
}

bool NativeWindow::IsTabletMode() const {
  return false;
}

std::string NativeWindow::GetRepresentedFilename() const {
  return "";
}

bool NativeWindow::IsDocumentEdited() const {
  return false;
}

bool NativeWindow::IsFocusable() const {
  return false;
}

void NativeWindow::SetParentWindow(NativeWindow* parent) {
  parent_ = parent;
}

bool NativeWindow::AddTabbedWindow(NativeWindow* window) {
  return true;  // for non-Mac platforms
}

std::optional<std::string> NativeWindow::GetTabbingIdentifier() const {
  return "";  // for non-Mac platforms
}

void NativeWindow::SetVibrancy(const std::string& type, int duration) {
  vibrancy_ = type;
}

void NativeWindow::SetBackgroundMaterial(const std::string& type) {
  background_material_ = type;
}

void NativeWindow::SetTouchBar(
    std::vector<gin_helper::PersistentDictionary> items) {}

void NativeWindow::SetEscapeTouchBarItem(
    gin_helper::PersistentDictionary item) {}

bool NativeWindow::IsMenuBarAutoHide() const {
  return false;
}

bool NativeWindow::IsMenuBarVisible() const {
  return true;
}

void NativeWindow::SetAspectRatio(double aspect_ratio,
                                  const gfx::Size& extra_size) {
  aspect_ratio_ = aspect_ratio;
  aspect_ratio_extraSize_ = extra_size;
}

bool NativeWindow::IsSnapped() const {
  return false;
}

std::optional<gfx::Rect> NativeWindow::GetWindowControlsOverlayRect() {
  return overlay_rect_;
}

void NativeWindow::SetWindowControlsOverlayRect(const gfx::Rect& overlay_rect) {
  overlay_rect_ = overlay_rect;
}

void NativeWindow::NotifyWindowRequestPreferredWidth(int* width) {
  observers_.Notify(&NativeWindowObserver::RequestPreferredWidth, width);
}

void NativeWindow::NotifyWindowCloseButtonClicked() {
  // First ask the observers whether we want to close.
  bool prevent_default = false;
  observers_.Notify(&NativeWindowObserver::WillCloseWindow, &prevent_default);
  if (prevent_default) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

  // Then ask the observers how should we close the window.
  observers_.Notify(&NativeWindowObserver::OnCloseButtonClicked,
                    &prevent_default);
  if (prevent_default)
    return;

  CloseImmediately();
}

void NativeWindow::NotifyWindowClosed() {
  if (is_closed_)
    return;

  is_closed_ = true;
  observers_.Notify(&NativeWindowObserver::OnWindowClosed);

  WindowList::RemoveWindow(this);
}

void NativeWindow::NotifyWindowQueryEndSession(
    const std::vector<std::string>& reasons,
    bool* prevent_default) {
  observers_.Notify(&NativeWindowObserver::OnWindowQueryEndSession, reasons,
                    prevent_default);
}

void NativeWindow::NotifyWindowEndSession(
    const std::vector<std::string>& reasons) {
  observers_.Notify(&NativeWindowObserver::OnWindowEndSession, reasons);
}

void NativeWindow::NotifyWindowBlur() {
  observers_.Notify(&NativeWindowObserver::OnWindowBlur);
}

void NativeWindow::NotifyWindowFocus() {
  observers_.Notify(&NativeWindowObserver::OnWindowFocus);
}

void NativeWindow::NotifyWindowIsKeyChanged(bool is_key) {
  observers_.Notify(&NativeWindowObserver::OnWindowIsKeyChanged, is_key);
}

void NativeWindow::NotifyWindowShow() {
  observers_.Notify(&NativeWindowObserver::OnWindowShow);
}

void NativeWindow::NotifyWindowHide() {
  observers_.Notify(&NativeWindowObserver::OnWindowHide);
}

void NativeWindow::NotifyWindowMaximize() {
  observers_.Notify(&NativeWindowObserver::OnWindowMaximize);
}

void NativeWindow::NotifyWindowUnmaximize() {
  observers_.Notify(&NativeWindowObserver::OnWindowUnmaximize);
}

void NativeWindow::NotifyWindowMinimize() {
  observers_.Notify(&NativeWindowObserver::OnWindowMinimize);
}

void NativeWindow::NotifyWindowRestore() {
  observers_.Notify(&NativeWindowObserver::OnWindowRestore);
}

void NativeWindow::NotifyWindowWillResize(const gfx::Rect& new_bounds,
                                          const gfx::ResizeEdge edge,
                                          bool* prevent_default) {
  observers_.Notify(&NativeWindowObserver::OnWindowWillResize, new_bounds, edge,
                    prevent_default);
}

void NativeWindow::NotifyWindowWillMove(const gfx::Rect& new_bounds,
                                        bool* prevent_default) {
  observers_.Notify(&NativeWindowObserver::OnWindowWillMove, new_bounds,
                    prevent_default);
}

void NativeWindow::NotifyWindowResize() {
  NotifyLayoutWindowControlsOverlay();
  observers_.Notify(&NativeWindowObserver::OnWindowResize);
}

void NativeWindow::NotifyWindowResized() {
  observers_.Notify(&NativeWindowObserver::OnWindowResized);
}

void NativeWindow::NotifyWindowMove() {
  observers_.Notify(&NativeWindowObserver::OnWindowMove);
}

void NativeWindow::NotifyWindowMoved() {
  observers_.Notify(&NativeWindowObserver::OnWindowMoved);
}

void NativeWindow::NotifyWindowEnterFullScreen() {
  NotifyLayoutWindowControlsOverlay();
  observers_.Notify(&NativeWindowObserver::OnWindowEnterFullScreen);
}

void NativeWindow::NotifyWindowSwipe(const std::string& direction) {
  observers_.Notify(&NativeWindowObserver::OnWindowSwipe, direction);
}

void NativeWindow::NotifyWindowRotateGesture(float rotation) {
  observers_.Notify(&NativeWindowObserver::OnWindowRotateGesture, rotation);
}

void NativeWindow::NotifyWindowSheetBegin() {
  observers_.Notify(&NativeWindowObserver::OnWindowSheetBegin);
}

void NativeWindow::NotifyWindowSheetEnd() {
  observers_.Notify(&NativeWindowObserver::OnWindowSheetEnd);
}

void NativeWindow::NotifyWindowLeaveFullScreen() {
  NotifyLayoutWindowControlsOverlay();
  observers_.Notify(&NativeWindowObserver::OnWindowLeaveFullScreen);
}

void NativeWindow::NotifyWindowEnterHtmlFullScreen() {
  observers_.Notify(&NativeWindowObserver::OnWindowEnterHtmlFullScreen);
}

void NativeWindow::NotifyWindowLeaveHtmlFullScreen() {
  observers_.Notify(&NativeWindowObserver::OnWindowLeaveHtmlFullScreen);
}

void NativeWindow::NotifyWindowAlwaysOnTopChanged() {
  observers_.Notify(&NativeWindowObserver::OnWindowAlwaysOnTopChanged);
}

void NativeWindow::NotifyWindowExecuteAppCommand(
    const std::string_view command_name) {
  observers_.Notify(&NativeWindowObserver::OnExecuteAppCommand, command_name);
}

void NativeWindow::NotifyTouchBarItemInteraction(const std::string& item_id,
                                                 base::Value::Dict details) {
  observers_.Notify(&NativeWindowObserver::OnTouchBarItemResult, item_id,
                    details);
}

void NativeWindow::NotifyNewWindowForTab() {
  observers_.Notify(&NativeWindowObserver::OnNewWindowForTab);
}

void NativeWindow::NotifyWindowSystemContextMenu(int x,
                                                 int y,
                                                 bool* prevent_default) {
  observers_.Notify(&NativeWindowObserver::OnSystemContextMenu, x, y,
                    prevent_default);
}

void NativeWindow::NotifyLayoutWindowControlsOverlay() {
  if (const auto bounds = GetWindowControlsOverlayRect())
    observers_.Notify(&NativeWindowObserver::UpdateWindowControlsOverlay,
                      *bounds);
}

#if BUILDFLAG(IS_WIN)
void NativeWindow::NotifyWindowMessage(UINT message,
                                       WPARAM w_param,
                                       LPARAM l_param) {
  observers_.Notify(&NativeWindowObserver::OnWindowMessage, message, w_param,
                    l_param);
}
#endif

int NativeWindow::NonClientHitTest(const gfx::Point& point) {
#if !BUILDFLAG(IS_MAC)
  // We need to ensure we account for resizing borders on Windows and Linux.
  if ((!has_frame() || has_client_frame()) && IsResizable()) {
    auto* frame =
        static_cast<FramelessView*>(widget()->non_client_view()->frame_view());
    int border_hit = frame->ResizingBorderHitTest(point);
    if (border_hit != HTNOWHERE)
      return border_hit;
  }
#endif

  // This is to disable dragging in HTML5 full screen mode.
  // Details: https://github.com/electron/electron/issues/41002
  if (widget()->IsFullscreen())
    return HTNOWHERE;

  for (auto* provider : draggable_region_providers_) {
    int hit = provider->NonClientHitTest(point);
    if (hit != HTNOWHERE)
      return hit;
  }
  return HTNOWHERE;
}

void NativeWindow::AddDraggableRegionProvider(
    DraggableRegionProvider* provider) {
  if (!base::Contains(draggable_region_providers_, provider)) {
    draggable_region_providers_.push_back(provider);
  }
}

void NativeWindow::RemoveDraggableRegionProvider(
    DraggableRegionProvider* provider) {
  draggable_region_providers_.remove_if(
      [&provider](DraggableRegionProvider* p) { return p == provider; });
}

void NativeWindow::AddBackgroundThrottlingSource(
    BackgroundThrottlingSource* source) {
  auto result = background_throttling_sources_.insert(source);
  DCHECK(result.second) << "Added already stored BackgroundThrottlingSource.";
  UpdateBackgroundThrottlingState();
}

void NativeWindow::RemoveBackgroundThrottlingSource(
    BackgroundThrottlingSource* source) {
  auto result = background_throttling_sources_.erase(source);
  DCHECK(result == 1)
      << "Tried to remove non existing BackgroundThrottlingSource.";
  UpdateBackgroundThrottlingState();
}

void NativeWindow::UpdateBackgroundThrottlingState() {
  if (!widget() || !widget()->GetCompositor()) {
    return;
  }
  bool enable_background_throttling = true;
  for (const auto* background_throttling_source :
       background_throttling_sources_) {
    if (!background_throttling_source->GetBackgroundThrottling()) {
      enable_background_throttling = false;
      break;
    }
  }
  widget()->GetCompositor()->SetBackgroundThrottling(
      enable_background_throttling);
}

views::Widget* NativeWindow::GetWidget() {
  return widget();
}

const views::Widget* NativeWindow::GetWidget() const {
  return widget();
}

std::string NativeWindow::GetTitle() const {
  return base::UTF16ToUTF8(WidgetDelegate::GetWindowTitle());
}

void NativeWindow::SetTitle(const std::string_view title) {
  if (title == GetTitle())
    return;

  WidgetDelegate::SetTitle(base::UTF8ToUTF16(title));
  OnTitleChanged();
}

void NativeWindow::SetAccessibleTitle(const std::string& title) {
  WidgetDelegate::SetAccessibleTitle(base::UTF8ToUTF16(title));
}

std::string NativeWindow::GetAccessibleTitle() {
  return base::UTF16ToUTF8(GetAccessibleWindowTitle());
}

void NativeWindow::HandlePendingFullscreenTransitions() {
  if (pending_transitions_.empty()) {
    set_fullscreen_transition_type(FullScreenTransitionType::kNone);
    return;
  }

  bool next_transition = pending_transitions_.front();
  pending_transitions_.pop();
  SetFullScreen(next_transition);
}

bool NativeWindow::IsTranslucent() const {
  // Transparent windows are translucent
  if (transparent()) {
    return true;
  }

#if BUILDFLAG(IS_MAC)
  // Windows with vibrancy set are translucent
  if (!vibrancy_.empty())
    return true;
#endif

#if BUILDFLAG(IS_WIN)
  // Windows with certain background materials may be translucent
  if (!background_material_.empty() && background_material_ != "none")
    return true;
#endif

  return false;
}

void NativeWindow::DebouncedSaveWindowState() {
  save_window_state_timer_.Start(
      FROM_HERE, base::Milliseconds(200),
      base::BindOnce(&NativeWindow::SaveWindowState, base::Unretained(this)));
}

void NativeWindow::SaveWindowState() {
  if (!prefs_ || window_name_.empty() || is_being_restored_)
    return;

  gfx::Rect bounds = GetBounds();
  const display::Screen* screen = display::Screen::GetScreen();
  DCHECK(screen);
  // GetDisplayMatching returns a fake display with 1920x1080 resolution at
  // (0,0) when no physical displays are attached.
  // https://source.chromium.org/chromium/chromium/src/+/main:ui/display/display.cc;l=184;drc=e4f1aef5f3ec30a28950d766612cc2c04c822c71
  const display::Display display = screen->GetDisplayMatching(bounds);

  // Skip window state persistence when display has invalid dimensions (0x0) or
  // is fake (ID 0xFF). Invalid displays could cause incorrect window bounds to
  // be saved, leading to positioning issues during restoration.
  // https://source.chromium.org/chromium/chromium/src/+/main:ui/display/types/display_constants.h;l=28;drc=e4f1aef5f3ec30a28950d766612cc2c04c822c71
  if (display.id() == display::kDefaultDisplayId ||
      display.size().width() == 0 || display.size().height() == 0) {
    LOG(WARNING)
        << "Window state not saved - no physical display attached or current "
           "display has invalid bounds";
    return;
  }

  ScopedDictPrefUpdate update(prefs_, electron::kWindowStates);
  const base::Value::Dict* existing_prefs = update->FindDict(window_name_);

  // When the window is in a special display mode (fullscreen, kiosk, or
  // maximized), save the previously stored window bounds instead of
  // the current bounds. This ensures that when the window is restored, it can
  // be restored to its original position and size if display mode is not
  // preserved via windowStatePersistence.
  if (!IsNormal() && existing_prefs) {
    std::optional<int> left = existing_prefs->FindInt(electron::kLeft);
    std::optional<int> top = existing_prefs->FindInt(electron::kTop);
    std::optional<int> right = existing_prefs->FindInt(electron::kRight);
    std::optional<int> bottom = existing_prefs->FindInt(electron::kBottom);

    if (left && top && right && bottom) {
      bounds = gfx::Rect(*left, *top, *right - *left, *bottom - *top);
    }
  }

  base::Value::Dict window_preferences;
  window_preferences.Set(electron::kLeft, bounds.x());
  window_preferences.Set(electron::kTop, bounds.y());
  window_preferences.Set(electron::kRight, bounds.right());
  window_preferences.Set(electron::kBottom, bounds.bottom());

  window_preferences.Set(electron::kMaximized, IsMaximized());
  window_preferences.Set(electron::kFullscreen, IsFullscreen());
  window_preferences.Set(electron::kKiosk, IsKiosk());

  gfx::Rect work_area = display.work_area();

  window_preferences.Set(electron::kWorkAreaLeft, work_area.x());
  window_preferences.Set(electron::kWorkAreaTop, work_area.y());
  window_preferences.Set(electron::kWorkAreaRight, work_area.right());
  window_preferences.Set(electron::kWorkAreaBottom, work_area.bottom());

  update->Set(window_name_, std::move(window_preferences));
}

void NativeWindow::FlushWindowState() {
  if (save_window_state_timer_.IsRunning()) {
    save_window_state_timer_.FireNow();
  } else {
    SaveWindowState();
  }
}

void NativeWindow::RestoreWindowState(const gin_helper::Dictionary& options) {
  is_being_restored_ = true;
  const base::Value& value = prefs_->GetValue(electron::kWindowStates);
  const base::Value::Dict* window_preferences =
      value.is_dict() ? value.GetDict().FindDict(window_name_) : nullptr;

  if (!window_preferences) {
    is_being_restored_ = false;
    return;
  }

  std::optional<int> saved_left = window_preferences->FindInt(electron::kLeft);
  std::optional<int> saved_top = window_preferences->FindInt(electron::kTop);
  std::optional<int> saved_right =
      window_preferences->FindInt(electron::kRight);
  std::optional<int> saved_bottom =
      window_preferences->FindInt(electron::kBottom);

  std::optional<int> work_area_left =
      window_preferences->FindInt(electron::kWorkAreaLeft);
  std::optional<int> work_area_top =
      window_preferences->FindInt(electron::kWorkAreaTop);
  std::optional<int> work_area_right =
      window_preferences->FindInt(electron::kWorkAreaRight);
  std::optional<int> work_area_bottom =
      window_preferences->FindInt(electron::kWorkAreaBottom);

  if (!saved_left || !saved_top || !saved_right || !saved_bottom ||
      !work_area_left || !work_area_top || !work_area_right ||
      !work_area_bottom) {
    LOG(WARNING) << "Window state not restored - corrupted values found";
    is_being_restored_ = false;
    return;
  }

  gfx::Rect saved_bounds =
      gfx::Rect(*saved_left, *saved_top, *saved_right - *saved_left,
                *saved_bottom - *saved_top);

  display::Screen* screen = display::Screen::GetScreen();
  DCHECK(screen);
  // GetDisplayMatching returns a fake display with 1920x1080 resolution at
  // (0,0) when no physical displays are attached.
  // https://source.chromium.org/chromium/chromium/src/+/main:ui/display/display.cc;l=184;drc=e4f1aef5f3ec30a28950d766612cc2c04c822c71
  const display::Display display = screen->GetDisplayMatching(saved_bounds);

  // Skip window state restoration if current display has invalid dimensions or
  // is fake. Restoring from invalid displays (0x0) or fake displays (ID 0xFF)
  // could cause incorrect window positioning when later moved to real displays.
  // https://source.chromium.org/chromium/chromium/src/+/main:ui/display/types/display_constants.h;l=28;drc=e4f1aef5f3ec30a28950d766612cc2c04c822c71
  if (display.id() == display::kDefaultDisplayId ||
      display.size().width() == 0 || display.size().height() == 0) {
    LOG(WARNING) << "Window state not restored - no physical display attached "
                    "or current display has invalid bounds";
    is_being_restored_ = false;
    return;
  }

  gfx::Rect saved_work_area = gfx::Rect(*work_area_left, *work_area_top,
                                        *work_area_right - *work_area_left,
                                        *work_area_bottom - *work_area_top);

  if (restore_bounds_) {
    RestoreBounds(display, saved_work_area, saved_bounds);
  }

  if (restore_display_mode_) {
    restore_display_mode_callback_ = base::BindOnce(
        [](NativeWindow* window, base::Value::Dict prefs) {
          if (auto kiosk = prefs.FindBool(electron::kKiosk); kiosk && *kiosk) {
            window->SetKiosk(true);
          } else if (auto fs = prefs.FindBool(electron::kFullscreen);
                     fs && *fs) {
            window->SetFullScreen(true);
          } else if (auto max = prefs.FindBool(electron::kMaximized);
                     max && *max) {
            window->Maximize();
          }
        },
        base::Unretained(this), window_preferences->Clone());
  }

  is_being_restored_ = false;
}

void NativeWindow::FlushPendingDisplayMode() {
  if (restore_display_mode_callback_) {
    std::move(restore_display_mode_callback_).Run();
  }
}

// This function is similar to Chromium's window bounds adjustment logic
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/window_sizer/window_sizer.cc;l=350;drc=0ec56065ba588552f21633aa47280ba02c3cd160
void NativeWindow::RestoreBounds(const display::Display& display,
                                 const gfx::Rect& saved_work_area,
                                 gfx::Rect& saved_bounds) {
  // Ensure that the window is at least kMinVisibleHeight * kMinVisibleWidth.
  saved_bounds.set_height(std::max(kMinVisibleHeight, saved_bounds.height()));
  saved_bounds.set_width(std::max(kMinVisibleWidth, saved_bounds.width()));

  const gfx::Rect work_area = display.work_area();
  // Ensure that the title bar is not above the work area.
  if (saved_bounds.y() < work_area.y()) {
    saved_bounds.set_y(work_area.y());
  }

  // Reposition and resize the bounds if the saved_work_area is different from
  // the current work area and the current work area doesn't completely contain
  // the bounds.
  if (!saved_work_area.IsEmpty() && saved_work_area != work_area &&
      !work_area.Contains(saved_bounds)) {
    saved_bounds.AdjustToFit(work_area);
  }

#if BUILDFLAG(IS_MAC)
  // On mac, we want to be aggressive about repositioning windows that are
  // partially offscreen.  If the window is partially offscreen horizontally,
  // snap to the nearest edge of the work area. This call also adjusts the
  // height, width if needed to make the window fully visible.
  saved_bounds.AdjustToFit(work_area);
#else
  // On non-Mac platforms, we are less aggressive about repositioning. Simply
  // ensure that at least kMinVisibleWidth * kMinVisibleHeight is visible

  const int min_y = work_area.y() + kMinVisibleHeight - saved_bounds.height();
  const int min_x = work_area.x() + kMinVisibleWidth - saved_bounds.width();
  const int max_y = work_area.bottom() - kMinVisibleHeight;
  const int max_x = work_area.right() - kMinVisibleWidth;
  // Reposition and resize the bounds to make it fully visible inside the work
  // area. `min_x >= max_x` happens when work area and bounds are both small.
  if (min_x >= max_x || min_y >= max_y) {
    saved_bounds.AdjustToFit(work_area);
  } else {
    saved_bounds.set_y(std::clamp(saved_bounds.y(), min_y, max_y));
    saved_bounds.set_x(std::clamp(saved_bounds.x(), min_x, max_x));
  }
#endif  // BUILDFLAG(IS_MAC)

  SetBounds(saved_bounds);
}

// static
bool NativeWindow::PlatformHasClientFrame() {
#if defined(USE_OZONE)
  // Ozone X11 likes to prefer custom frames,
  // but we don't need them unless on Wayland.
  static const bool has_client_frame =
      base::FeatureList::IsEnabled(features::kWaylandWindowDecorations) &&
      !ui::OzonePlatform::GetInstance()
           ->GetPlatformRuntimeProperties()
           .supports_server_side_window_decorations;
  return has_client_frame;
#else
  return false;
#endif
}

// static
void NativeWindowRelay::CreateForWebContents(
    content::WebContents* web_contents,
    base::WeakPtr<NativeWindow> window) {
  DCHECK(web_contents);
  if (!web_contents->GetUserData(UserDataKey())) {
    web_contents->SetUserData(
        UserDataKey(),
        base::WrapUnique(new NativeWindowRelay(web_contents, window)));
  }
}

NativeWindowRelay::NativeWindowRelay(content::WebContents* web_contents,
                                     base::WeakPtr<NativeWindow> window)
    : content::WebContentsUserData<NativeWindowRelay>(*web_contents),
      native_window_(window) {}

NativeWindowRelay::~NativeWindowRelay() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(NativeWindowRelay);

}  // namespace electron
