// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window.h"

#include <string>
#include <utility>
#include <vector>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/browser.h"
#include "atom/browser/window_list.h"
#include "atom/common/color_util.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/options_switches.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_switches.h"
#include "ipc/ipc_message_macros.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_conversions.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::NativeWindowRelay);

namespace atom {

NativeWindow::NativeWindow(
    brightray::InspectableWebContents* inspectable_web_contents,
    const mate::Dictionary& options,
    NativeWindow* parent)
    : content::WebContentsObserver(inspectable_web_contents->GetWebContents()),
      has_frame_(true),
      transparent_(false),
      enable_larger_than_screen_(false),
      is_closed_(false),
      sheet_offset_x_(0.0),
      sheet_offset_y_(0.0),
      aspect_ratio_(0.0),
      parent_(parent),
      is_modal_(false),
      is_osr_dummy_(false),
      browser_view_(nullptr),
      inspectable_web_contents_(inspectable_web_contents),
      weak_factory_(this) {
  options.Get(options::kFrame, &has_frame_);
  options.Get(options::kTransparent, &transparent_);
  options.Get(options::kEnableLargerThanScreen, &enable_larger_than_screen_);

  if (parent)
    options.Get("modal", &is_modal_);

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
  for (const auto& window : WindowList::GetWindows()) {
    if (window->web_contents() == web_contents)
      return window;
  }
  return nullptr;
}

void NativeWindow::InitFromOptions(const mate::Dictionary& options) {
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
  // On Linux and Window we may already have maximum size defined.
  extensions::SizeConstraints size_constraints(GetContentSizeConstraints());
  int min_height = 0, min_width = 0;
  if (options.Get(options::kMinHeight, &min_height) |
      options.Get(options::kMinWidth, &min_width)) {
    size_constraints.set_minimum_size(gfx::Size(min_width, min_height));
  }
  int max_height = INT_MAX, max_width = INT_MAX;
  if (options.Get(options::kMaxHeight, &max_height) |
      options.Get(options::kMaxWidth, &max_width)) {
    size_constraints.set_maximum_size(gfx::Size(max_width, max_height));
  }
  bool use_content_size = false;
  options.Get(options::kUseContentSize, &use_content_size);
  if (use_content_size) {
    SetContentSizeConstraints(size_constraints);
  } else {
    SetSizeConstraints(size_constraints);
  }
#if defined(OS_WIN) || defined(USE_X11)
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
    SetAlwaysOnTop(true);
  }
  bool fullscreenable = true;
  bool fullscreen = false;
  if (options.Get(options::kFullscreen, &fullscreen) && !fullscreen) {
    // Disable fullscreen button if 'fullscreen' is specified to false.
  #if defined(OS_MACOSX)
    fullscreenable = false;
  #endif
  }
  // Overriden by 'fullscreenable'.
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

void NativeWindow::SetFocusable(bool focusable) {
}

void NativeWindow::SetMenu(AtomMenuModel* menu) {
}

void NativeWindow::SetParentWindow(NativeWindow* parent) {
  parent_ = parent;
}

void NativeWindow::SetAutoHideCursor(bool auto_hide) {
}

void NativeWindow::SelectPreviousTab() {
}

void NativeWindow::SelectNextTab() {
}

void NativeWindow::MergeAllWindows() {
}

void NativeWindow::MoveTabToNewWindow() {
}

void NativeWindow::ToggleTabBar() {
}

bool NativeWindow::AddTabbedWindow(NativeWindow* window) {
  return true;  // for non-Mac platforms
}

void NativeWindow::SetVibrancy(const std::string& filename) {
}

void NativeWindow::SetTouchBar(
    const std::vector<mate::PersistentDictionary>& items) {
}

void NativeWindow::RefreshTouchBarItem(const std::string& item_id) {
}

void NativeWindow::SetEscapeTouchBarItem(
    const mate::PersistentDictionary& item) {
}

void NativeWindow::FocusOnWebView() {
  web_contents()->GetRenderViewHost()->GetWidget()->Focus();
}

void NativeWindow::BlurWebView() {
  web_contents()->GetRenderViewHost()->GetWidget()->Blur();
}

bool NativeWindow::IsWebViewFocused() {
  auto host_view = web_contents()->GetRenderViewHost()->GetWidget()->GetView();
  return host_view && host_view->HasFocus();
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

void NativeWindow::PreviewFile(const std::string& path,
                               const std::string& display_name) {
}

void NativeWindow::CloseFilePreview() {
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

  WindowList::RemoveWindow(this);

  is_closed_ = true;
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowClosed();
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

void NativeWindow::NotifyWindowResize() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowResize();
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

void NativeWindow::NotifyWindowExecuteWindowsCommand(
    const std::string& command) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnExecuteWindowsCommand(command);
}

void NativeWindow::NotifyTouchBarItemInteraction(
    const std::string& item_id,
    const base::DictionaryValue& details) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnTouchBarItemResult(item_id, details);
}

void NativeWindow::NotifyNewWindowForTab() {
  for (NativeWindowObserver &observer : observers_)
    observer.OnNewWindowForTab();
}

#if defined(OS_WIN)
void NativeWindow::NotifyWindowMessage(
    UINT message, WPARAM w_param, LPARAM l_param) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMessage(message, w_param, l_param);
}
#endif

}  // namespace atom
