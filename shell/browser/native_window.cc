// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_window.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window_features.h"
#include "shell/browser/window_list.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/persistent_dictionary.h"
#include "shell/common/options_switches.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "ui/base/win/shell.h"
#include "ui/display/win/screen_win.h"
#endif

#if defined(USE_OZONE)
#include "ui/base/ui_base_features.h"
#include "ui/ozone/public/ozone_platform.h"
#endif

namespace gin {

template <>
struct Converter<electron::NativeWindow::TitleBarStyle> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     electron::NativeWindow::TitleBarStyle* out) {
    using TitleBarStyle = electron::NativeWindow::TitleBarStyle;
    std::string title_bar_style;
    if (!ConvertFromV8(isolate, val, &title_bar_style))
      return false;
    if (title_bar_style == "hidden") {
      *out = TitleBarStyle::kHidden;
#if defined(OS_MAC)
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

#if defined(OS_WIN)
gfx::Size GetExpandedWindowSize(const NativeWindow* window, gfx::Size size) {
  if (!window->transparent() || !ui::win::IsAeroGlassEnabled())
    return size;

  gfx::Size min_size = display::win::ScreenWin::ScreenToDIPSize(
      window->GetAcceleratedWidget(), gfx::Size(64, 64));

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
    : widget_(std::make_unique<views::Widget>()), parent_(parent) {
  ++next_id_;

  options.Get(options::kFrame, &has_frame_);
  options.Get(options::kTransparent, &transparent_);
  options.Get(options::kEnableLargerThanScreen, &enable_larger_than_screen_);
  options.Get(options::kTitleBarStyle, &title_bar_style_);

  v8::Local<v8::Value> titlebar_overlay;
  if (options.Get(options::ktitleBarOverlay, &titlebar_overlay)) {
    if (titlebar_overlay->IsBoolean()) {
      options.Get(options::ktitleBarOverlay, &titlebar_overlay_);
    } else if (titlebar_overlay->IsObject()) {
      titlebar_overlay_ = true;

      gin_helper::Dictionary titlebar_overlay =
          gin::Dictionary::CreateEmpty(options.isolate());
      options.Get(options::ktitleBarOverlay, &titlebar_overlay);
      int height;
      if (titlebar_overlay.Get(options::kOverlayHeight, &height))
        titlebar_overlay_height_ = height;

#if !(defined(OS_WIN) || defined(OS_MAC))
      DCHECK(false);
#endif
    }
  }

  if (parent)
    options.Get("modal", &is_modal_);

#if defined(USE_OZONE)
  // Ozone X11 likes to prefer custom frames, but we don't need them unless
  // on Wayland.
  if (base::FeatureList::IsEnabled(features::kWaylandWindowDecorations) &&
      !ui::OzonePlatform::GetInstance()
           ->GetPlatformRuntimeProperties()
           .supports_server_side_window_decorations) {
    has_client_frame_ = true;
  }
#endif

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
  int x = -1, y = -1;
  bool center;
  if (options.Get(options::kX, &x) && options.Get(options::kY, &y)) {
    SetPosition(gfx::Point(x, y));

#if defined(OS_WIN)
    // FIXME(felixrieseberg): Dirty, dirty workaround for
    // https://github.com/electron/electron/issues/10862
    // Somehow, we need to call `SetBounds` twice to get
    // usable results. The root cause is still unknown.
    SetPosition(gfx::Point(x, y));
#endif
  } else if (options.Get(options::kCenter, &center) && center) {
    Center();
  }

  bool use_content_size = false;
  options.Get(options::kUseContentSize, &use_content_size);

  // On Linux and Window we may already have maximum size defined.
  extensions::SizeConstraints size_constraints(
      use_content_size ? GetContentSizeConstraints() : GetSizeConstraints());

  int min_width = size_constraints.GetMinimumSize().width();
  int min_height = size_constraints.GetMinimumSize().height();
  options.Get(options::kMinWidth, &min_width);
  options.Get(options::kMinHeight, &min_height);
  size_constraints.set_minimum_size(gfx::Size(min_width, min_height));

  gfx::Size max_size = size_constraints.GetMaximumSize();
  int max_width = max_size.width() > 0 ? max_size.width() : INT_MAX;
  int max_height = max_size.height() > 0 ? max_size.height() : INT_MAX;
  bool have_max_width = options.Get(options::kMaxWidth, &max_width);
  bool have_max_height = options.Get(options::kMaxHeight, &max_height);

  // By default the window has a default maximum size that prevents it
  // from being resized larger than the screen, so we should only set this
  // if th user has passed in values.
  if (have_max_height || have_max_width || !max_size.IsEmpty())
    size_constraints.set_maximum_size(gfx::Size(max_width, max_height));

  if (use_content_size) {
    SetContentSizeConstraints(size_constraints);
  } else {
    SetSizeConstraints(size_constraints);
  }
#if defined(OS_WIN) || defined(OS_LINUX)
  bool resizable;
  if (options.Get(options::kResizable, &resizable)) {
    SetResizable(resizable);
  }
  bool closable;
  if (options.Get(options::kClosable, &closable)) {
    SetClosable(closable);
  }
#endif
  bool movable;
  if (options.Get(options::kMovable, &movable)) {
    SetMovable(movable);
  }
  bool has_shadow;
  if (options.Get(options::kHasShadow, &has_shadow)) {
    SetHasShadow(has_shadow);
  }
  double opacity;
  if (options.Get(options::kOpacity, &opacity)) {
    SetOpacity(opacity);
  }
  bool top;
  if (options.Get(options::kAlwaysOnTop, &top) && top) {
    SetAlwaysOnTop(ui::ZOrderLevel::kFloatingWindow);
  }
  bool fullscreenable = true;
  bool fullscreen = false;
  if (options.Get(options::kFullscreen, &fullscreen) && !fullscreen) {
    // Disable fullscreen button if 'fullscreen' is specified to false.
#if defined(OS_MAC)
    fullscreenable = false;
#endif
  }
  // Overridden by 'fullscreenable'.
  options.Get(options::kFullScreenable, &fullscreenable);
  SetFullScreenable(fullscreenable);
  if (fullscreen) {
    SetFullScreen(true);
  }
  bool skip;
  if (options.Get(options::kSkipTaskbar, &skip)) {
    SetSkipTaskbar(skip);
  }
  bool kiosk;
  if (options.Get(options::kKiosk, &kiosk) && kiosk) {
    SetKiosk(kiosk);
  }
#if defined(OS_MAC)
  std::string type;
  if (options.Get(options::kVibrancyType, &type)) {
    SetVibrancy(type);
  }
#endif
  std::string color;
  if (options.Get(options::kBackgroundColor, &color)) {
    SetBackgroundColor(ParseHexColor(color));
  } else if (!transparent()) {
    // For normal window, use white as default background.
    SetBackgroundColor(SK_ColorWHITE);
  }
  std::string title(Browser::Get()->GetName());
  options.Get(options::kTitle, &title);
  SetTitle(title);

  // Then show it.
  bool show = true;
  options.Get(options::kShow, &show);
  if (show)
    Show();
}

bool NativeWindow::IsClosed() const {
  return is_closed_;
}

void NativeWindow::SetSize(const gfx::Size& size, bool animate) {
  SetBounds(gfx::Rect(GetPosition(), size), animate);
}

gfx::Size NativeWindow::GetSize() {
  return GetBounds().size();
}

void NativeWindow::SetPosition(const gfx::Point& position, bool animate) {
  SetBounds(gfx::Rect(position, GetSize()), animate);
}

gfx::Point NativeWindow::GetPosition() {
  return GetBounds().origin();
}

void NativeWindow::SetContentSize(const gfx::Size& size, bool animate) {
  SetSize(ContentBoundsToWindowBounds(gfx::Rect(size)).size(), animate);
}

gfx::Size NativeWindow::GetContentSize() {
  return GetContentBounds().size();
}

void NativeWindow::SetContentBounds(const gfx::Rect& bounds, bool animate) {
  SetBounds(ContentBoundsToWindowBounds(bounds), animate);
}

gfx::Rect NativeWindow::GetContentBounds() {
  return WindowBoundsToContentBounds(GetBounds());
}

bool NativeWindow::IsNormal() {
  return !IsMinimized() && !IsMaximized() && !IsFullscreen();
}

void NativeWindow::SetSizeConstraints(
    const extensions::SizeConstraints& window_constraints) {
  extensions::SizeConstraints content_constraints(GetContentSizeConstraints());
  if (window_constraints.HasMaximumSize()) {
    gfx::Rect max_bounds = WindowBoundsToContentBounds(
        gfx::Rect(window_constraints.GetMaximumSize()));
    content_constraints.set_maximum_size(max_bounds.size());
  }
  if (window_constraints.HasMinimumSize()) {
    gfx::Rect min_bounds = WindowBoundsToContentBounds(
        gfx::Rect(window_constraints.GetMinimumSize()));
    content_constraints.set_minimum_size(min_bounds.size());
  }
  SetContentSizeConstraints(content_constraints);
}

extensions::SizeConstraints NativeWindow::GetSizeConstraints() const {
  extensions::SizeConstraints content_constraints = GetContentSizeConstraints();
  extensions::SizeConstraints window_constraints;
  if (content_constraints.HasMaximumSize()) {
    gfx::Rect max_bounds = ContentBoundsToWindowBounds(
        gfx::Rect(content_constraints.GetMaximumSize()));
    window_constraints.set_maximum_size(max_bounds.size());
  }
  if (content_constraints.HasMinimumSize()) {
    gfx::Rect min_bounds = ContentBoundsToWindowBounds(
        gfx::Rect(content_constraints.GetMinimumSize()));
    window_constraints.set_minimum_size(min_bounds.size());
  }
  return window_constraints;
}

void NativeWindow::SetContentSizeConstraints(
    const extensions::SizeConstraints& size_constraints) {
  size_constraints_ = size_constraints;
}

extensions::SizeConstraints NativeWindow::GetContentSizeConstraints() const {
  return size_constraints_;
}

void NativeWindow::SetMinimumSize(const gfx::Size& size) {
  extensions::SizeConstraints size_constraints;
  size_constraints.set_minimum_size(size);
  SetSizeConstraints(size_constraints);
}

gfx::Size NativeWindow::GetMinimumSize() const {
  return GetSizeConstraints().GetMinimumSize();
}

void NativeWindow::SetMaximumSize(const gfx::Size& size) {
  extensions::SizeConstraints size_constraints;
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
  gfx::Size maximum_size = GetContentSizeConstraints().GetMaximumSize();
#if defined(OS_WIN)
  return GetContentSizeConstraints().HasMaximumSize()
             ? GetExpandedWindowSize(this, maximum_size)
             : maximum_size;
#else
  return maximum_size;
#endif
}

void NativeWindow::SetSheetOffset(const double offsetX, const double offsetY) {
  sheet_offset_x_ = offsetX;
  sheet_offset_y_ = offsetY;
}

double NativeWindow::GetSheetOffsetX() {
  return sheet_offset_x_;
}

double NativeWindow::GetSheetOffsetY() {
  return sheet_offset_y_;
}

bool NativeWindow::IsTabletMode() const {
  return false;
}

void NativeWindow::SetRepresentedFilename(const std::string& filename) {}

std::string NativeWindow::GetRepresentedFilename() {
  return "";
}

void NativeWindow::SetDocumentEdited(bool edited) {}

bool NativeWindow::IsDocumentEdited() {
  return false;
}

void NativeWindow::SetFocusable(bool focusable) {}

bool NativeWindow::IsFocusable() {
  return false;
}

void NativeWindow::SetMenu(ElectronMenuModel* menu) {}

void NativeWindow::SetParentWindow(NativeWindow* parent) {
  parent_ = parent;
}

void NativeWindow::SetAutoHideCursor(bool auto_hide) {}

void NativeWindow::SelectPreviousTab() {}

void NativeWindow::SelectNextTab() {}

void NativeWindow::MergeAllWindows() {}

void NativeWindow::MoveTabToNewWindow() {}

void NativeWindow::ToggleTabBar() {}

bool NativeWindow::AddTabbedWindow(NativeWindow* window) {
  return true;  // for non-Mac platforms
}

void NativeWindow::SetVibrancy(const std::string& type) {}

void NativeWindow::SetTouchBar(
    std::vector<gin_helper::PersistentDictionary> items) {}

void NativeWindow::RefreshTouchBarItem(const std::string& item_id) {}

void NativeWindow::SetEscapeTouchBarItem(
    gin_helper::PersistentDictionary item) {}

void NativeWindow::SetAutoHideMenuBar(bool auto_hide) {}

bool NativeWindow::IsMenuBarAutoHide() {
  return false;
}

void NativeWindow::SetMenuBarVisibility(bool visible) {}

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

void NativeWindow::PreviewFile(const std::string& path,
                               const std::string& display_name) {}

void NativeWindow::CloseFilePreview() {}

gfx::Rect NativeWindow::GetWindowControlsOverlayRect() {
  return overlay_rect_;
}

void NativeWindow::SetWindowControlsOverlayRect(const gfx::Rect& overlay_rect) {
  overlay_rect_ = overlay_rect;
}

void NativeWindow::NotifyWindowRequestPreferredWith(int* width) {
  for (NativeWindowObserver& observer : observers_)
    observer.RequestPreferredWidth(width);
}

void NativeWindow::NotifyWindowCloseButtonClicked() {
  // First ask the observers whether we want to close.
  bool prevent_default = false;
  for (NativeWindowObserver& observer : observers_)
    observer.WillCloseWindow(&prevent_default);
  if (prevent_default) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

  // Then ask the observers how should we close the window.
  for (NativeWindowObserver& observer : observers_)
    observer.OnCloseButtonClicked(&prevent_default);
  if (prevent_default)
    return;

  CloseImmediately();
}

void NativeWindow::NotifyWindowClosed() {
  if (is_closed_)
    return;

  is_closed_ = true;
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowClosed();

  WindowList::RemoveWindow(this);
}

void NativeWindow::NotifyWindowEndSession() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowEndSession();
}

void NativeWindow::NotifyWindowBlur() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowBlur();
}

void NativeWindow::NotifyWindowFocus() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowFocus();
}

void NativeWindow::NotifyWindowIsKeyChanged(bool is_key) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowIsKeyChanged(is_key);
}

void NativeWindow::NotifyWindowShow() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowShow();
}

void NativeWindow::NotifyWindowHide() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowHide();
}

void NativeWindow::NotifyWindowMaximize() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMaximize();
}

void NativeWindow::NotifyWindowUnmaximize() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowUnmaximize();
}

void NativeWindow::NotifyWindowMinimize() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMinimize();
}

void NativeWindow::NotifyWindowRestore() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowRestore();
}

void NativeWindow::NotifyWindowWillResize(const gfx::Rect& new_bounds,
                                          const gfx::ResizeEdge& edge,
                                          bool* prevent_default) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowWillResize(new_bounds, edge, prevent_default);
}

void NativeWindow::NotifyWindowWillMove(const gfx::Rect& new_bounds,
                                        bool* prevent_default) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowWillMove(new_bounds, prevent_default);
}

void NativeWindow::NotifyWindowResize() {
  NotifyLayoutWindowControlsOverlay();
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowResize();
}

void NativeWindow::NotifyWindowResized() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowResized();
}

void NativeWindow::NotifyWindowMove() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMove();
}

void NativeWindow::NotifyWindowMoved() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMoved();
}

void NativeWindow::NotifyWindowEnterFullScreen() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowEnterFullScreen();
}

void NativeWindow::NotifyWindowScrollTouchBegin() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowScrollTouchBegin();
}

void NativeWindow::NotifyWindowScrollTouchEnd() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowScrollTouchEnd();
}

void NativeWindow::NotifyWindowSwipe(const std::string& direction) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowSwipe(direction);
}

void NativeWindow::NotifyWindowRotateGesture(float rotation) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowRotateGesture(rotation);
}

void NativeWindow::NotifyWindowSheetBegin() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowSheetBegin();
}

void NativeWindow::NotifyWindowSheetEnd() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowSheetEnd();
}

void NativeWindow::NotifyWindowLeaveFullScreen() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowLeaveFullScreen();
}

void NativeWindow::NotifyWindowEnterHtmlFullScreen() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowEnterHtmlFullScreen();
}

void NativeWindow::NotifyWindowLeaveHtmlFullScreen() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowLeaveHtmlFullScreen();
}

void NativeWindow::NotifyWindowAlwaysOnTopChanged() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowAlwaysOnTopChanged();
}

void NativeWindow::NotifyWindowExecuteAppCommand(const std::string& command) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnExecuteAppCommand(command);
}

void NativeWindow::NotifyTouchBarItemInteraction(
    const std::string& item_id,
    const base::DictionaryValue& details) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnTouchBarItemResult(item_id, details);
}

void NativeWindow::NotifyNewWindowForTab() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnNewWindowForTab();
}

void NativeWindow::NotifyWindowSystemContextMenu(int x,
                                                 int y,
                                                 bool* prevent_default) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnSystemContextMenu(x, y, prevent_default);
}

void NativeWindow::NotifyLayoutWindowControlsOverlay() {
  gfx::Rect bounding_rect = GetWindowControlsOverlayRect();
  if (!bounding_rect.IsEmpty()) {
    for (NativeWindowObserver& observer : observers_)
      observer.UpdateWindowControlsOverlay(bounding_rect);
  }
}

#if defined(OS_WIN)
void NativeWindow::NotifyWindowMessage(UINT message,
                                       WPARAM w_param,
                                       LPARAM l_param) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMessage(message, w_param, l_param);
}
#endif

views::Widget* NativeWindow::GetWidget() {
  return widget();
}

const views::Widget* NativeWindow::GetWidget() const {
  return widget();
}

std::u16string NativeWindow::GetAccessibleWindowTitle() const {
  if (accessible_title_.empty()) {
    return views::WidgetDelegate::GetAccessibleWindowTitle();
  }

  return accessible_title_;
}

void NativeWindow::SetAccessibleTitle(const std::string& title) {
  accessible_title_ = base::UTF8ToUTF16(title);
}

std::string NativeWindow::GetAccessibleTitle() {
  return base::UTF16ToUTF8(accessible_title_);
}

// static
int32_t NativeWindow::next_id_ = 0;

// static
void NativeWindowRelay::CreateForWebContents(
    content::WebContents* web_contents,
    base::WeakPtr<NativeWindow> window) {
  DCHECK(web_contents);
  if (!web_contents->GetUserData(UserDataKey())) {
    web_contents->SetUserData(UserDataKey(),
                              base::WrapUnique(new NativeWindowRelay(window)));
  }
}

NativeWindowRelay::NativeWindowRelay(base::WeakPtr<NativeWindow> window)
    : native_window_(window) {}

NativeWindowRelay::~NativeWindowRelay() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(NativeWindowRelay);

}  // namespace electron
