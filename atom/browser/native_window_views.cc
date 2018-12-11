// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_views.h"

#if defined(OS_WIN)
#include <objbase.h>
#include <wrl/client.h>
#endif

#include <tuple>
#include <vector>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/native_browser_view_views.h"
#include "atom/browser/ui/views/root_view.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/web_view_manager.h"
#include "atom/browser/window_list.h"
#include "atom/common/draggable_region.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/options_switches.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/image/image.h"
#include "ui/views/background.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/client_view.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_util.h"

#if defined(USE_X11)
#include "atom/browser/browser.h"
#include "atom/browser/ui/views/frameless_view.h"
#include "atom/browser/ui/views/global_menu_bar_x11.h"
#include "atom/browser/ui/views/native_frame_view.h"
#include "atom/browser/ui/x/event_disabler.h"
#include "atom/browser/ui/x/window_state_watcher.h"
#include "atom/browser/ui/x/x_window_utils.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/libgtkui/unity_service.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_x11.h"
#include "ui/views/window/native_frame_view.h"
#elif defined(OS_WIN)
#include "atom/browser/ui/views/win_frame_view.h"
#include "atom/browser/ui/win/atom_desktop_native_widget_aura.h"
#include "atom/browser/ui/win/atom_desktop_window_tree_host_win.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/base/win/shell.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/win/screen_win.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

namespace atom {

namespace {

#if defined(OS_WIN)
void FlipWindowStyle(HWND handle, bool on, DWORD flag) {
  DWORD style = ::GetWindowLong(handle, GWL_STYLE);
  if (on)
    style |= flag;
  else
    style &= ~flag;
  ::SetWindowLong(handle, GWL_STYLE, style);
}
#endif

class NativeWindowClientView : public views::ClientView {
 public:
  NativeWindowClientView(views::Widget* widget,
                         views::View* root_view,
                         NativeWindowViews* window)
      : views::ClientView(widget, root_view), window_(window) {}
  ~NativeWindowClientView() override = default;

  bool CanClose() override {
    window_->NotifyWindowCloseButtonClicked();
    return false;
  }

 private:
  NativeWindowViews* window_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowClientView);
};

}  // namespace

NativeWindowViews::NativeWindowViews(const mate::Dictionary& options,
                                     NativeWindow* parent)
    : NativeWindow(options, parent),
      root_view_(new RootView(this)),
      keyboard_event_handler_(new views::UnhandledKeyboardEventHandler) {
  options.Get(options::kTitle, &title_);

  bool menu_bar_autohide;
  if (options.Get(options::kAutoHideMenuBar, &menu_bar_autohide))
    root_view_->SetAutoHideMenuBar(menu_bar_autohide);

#if defined(OS_WIN)
  // On Windows we rely on the CanResize() to indicate whether window can be
  // resized, and it should be set before window is created.
  options.Get(options::kResizable, &resizable_);
  options.Get(options::kMinimizable, &minimizable_);
  options.Get(options::kMaximizable, &maximizable_);

  // Transparent window must not have thick frame.
  options.Get("thickFrame", &thick_frame_);
  if (transparent())
    thick_frame_ = false;
#endif

  if (enable_larger_than_screen())
    // We need to set a default maximum window size here otherwise Windows
    // will not allow us to resize the window larger than scree.
    // Setting directly to INT_MAX somehow doesn't work, so we just devide
    // by 10, which should still be large enough.
    SetContentSizeConstraints(extensions::SizeConstraints(
        gfx::Size(), gfx::Size(INT_MAX / 10, INT_MAX / 10)));

  int width = 800, height = 600;
  options.Get(options::kWidth, &width);
  options.Get(options::kHeight, &height);
  gfx::Rect bounds(0, 0, width, height);
  widget_size_ = bounds.size();

  widget()->AddObserver(this);

  views::Widget::InitParams params;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.bounds = bounds;
  params.delegate = this;
  params.type = views::Widget::InitParams::TYPE_WINDOW;
  params.remove_standard_frame = !has_frame();

  if (transparent())
    params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;

  // The given window is most likely not rectangular since it uses
  // transparency and has no standard frame, don't show a shadow for it.
  if (transparent() && !has_frame())
    params.shadow_type = views::Widget::InitParams::SHADOW_TYPE_NONE;

  bool focusable;
  if (options.Get(options::kFocusable, &focusable) && !focusable)
    params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;

#if defined(OS_WIN)
  if (parent)
    params.parent = parent->GetNativeWindow();

  params.native_widget = new AtomDesktopNativeWidgetAura(widget());
  atom_desktop_window_tree_host_win_ = new AtomDesktopWindowTreeHostWin(
      this, widget(),
      static_cast<views::DesktopNativeWidgetAura*>(params.native_widget));
  params.desktop_window_tree_host = atom_desktop_window_tree_host_win_;
#elif defined(USE_X11)
  std::string name = Browser::Get()->GetName();
  // Set WM_WINDOW_ROLE.
  params.wm_role_name = "browser-window";
  // Set WM_CLASS.
  params.wm_class_name = base::ToLowerASCII(name);
  params.wm_class_class = name;
#endif

  widget()->Init(params);

  bool fullscreen = false;
  options.Get(options::kFullscreen, &fullscreen);

  std::string window_type;
  options.Get(options::kType, &window_type);

#if defined(USE_X11)
  // Start monitoring window states.
  window_state_watcher_.reset(new WindowStateWatcher(this));

  // Set _GTK_THEME_VARIANT to dark if we have "dark-theme" option set.
  bool use_dark_theme = false;
  if (options.Get(options::kDarkTheme, &use_dark_theme) && use_dark_theme) {
    XDisplay* xdisplay = gfx::GetXDisplay();
    XChangeProperty(xdisplay, GetAcceleratedWidget(),
                    XInternAtom(xdisplay, "_GTK_THEME_VARIANT", x11::False),
                    XInternAtom(xdisplay, "UTF8_STRING", x11::False), 8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char*>("dark"), 4);
  }

  // Before the window is mapped the SetWMSpecState can not work, so we have
  // to manually set the _NET_WM_STATE.
  std::vector<::Atom> state_atom_list;
  bool skip_taskbar = false;
  if (options.Get(options::kSkipTaskbar, &skip_taskbar) && skip_taskbar) {
    state_atom_list.push_back(GetAtom("_NET_WM_STATE_SKIP_TASKBAR"));
  }

  // Before the window is mapped, there is no SHOW_FULLSCREEN_STATE.
  if (fullscreen) {
    state_atom_list.push_back(GetAtom("_NET_WM_STATE_FULLSCREEN"));
  }

  if (parent) {
    SetParentWindow(parent);
    // Force using dialog type for child window.
    window_type = "dialog";
    // Modal window needs the _NET_WM_STATE_MODAL hint.
    if (is_modal())
      state_atom_list.push_back(GetAtom("_NET_WM_STATE_MODAL"));
  }

  if (!state_atom_list.empty())
    ui::SetAtomArrayProperty(GetAcceleratedWidget(), "_NET_WM_STATE", "ATOM",
                             state_atom_list);

  // Set the _NET_WM_WINDOW_TYPE.
  if (!window_type.empty())
    SetWindowType(GetAcceleratedWidget(), window_type);
#endif

#if defined(OS_WIN)
  if (!has_frame()) {
    // Set Window style so that we get a minimize and maximize animation when
    // frameless.
    DWORD frame_style = WS_CAPTION;
    if (resizable_)
      frame_style |= WS_THICKFRAME;
    if (minimizable_)
      frame_style |= WS_MINIMIZEBOX;
    if (maximizable_)
      frame_style |= WS_MAXIMIZEBOX;
    // We should not show a frame for transparent window.
    if (!thick_frame_)
      frame_style &= ~(WS_THICKFRAME | WS_CAPTION);
    ::SetWindowLong(GetAcceleratedWidget(), GWL_STYLE, frame_style);
  }

  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  if (window_type == "toolbar")
    ex_style |= WS_EX_TOOLWINDOW;
  ::SetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE, ex_style);
#endif

  if (has_frame()) {
    // TODO(zcbenz): This was used to force using native frame on Windows 2003,
    // we should check whether setting it in InitParams can work.
    widget()->set_frame_type(views::Widget::FrameType::FRAME_TYPE_FORCE_NATIVE);
    widget()->FrameTypeChanged();
#if defined(OS_WIN)
    // thickFrame also works for normal window.
    if (!thick_frame_)
      FlipWindowStyle(GetAcceleratedWidget(), false, WS_THICKFRAME);
#endif
  }

  // Default content view.
  SetContentView(new views::View());

  gfx::Size size = bounds.size();
  if (has_frame() &&
      options.Get(options::kUseContentSize, &use_content_size_) &&
      use_content_size_)
    size = ContentBoundsToWindowBounds(gfx::Rect(size)).size();

  widget()->CenterWindow(size);

#if defined(OS_WIN)
  // Save initial window state.
  if (fullscreen)
    last_window_state_ = ui::SHOW_STATE_FULLSCREEN;
  else
    last_window_state_ = ui::SHOW_STATE_NORMAL;
  last_normal_bounds_ = GetBounds();
#endif
}

NativeWindowViews::~NativeWindowViews() {
  widget()->RemoveObserver(this);

#if defined(OS_WIN)
  // Disable mouse forwarding to relinquish resources, should any be held.
  SetForwardMouseMessages(false);
#endif
}

void NativeWindowViews::SetContentView(views::View* view) {
  if (content_view()) {
    root_view_->RemoveChildView(content_view());
    if (browser_view()) {
      content_view()->RemoveChildView(
          browser_view()->GetInspectableWebContentsView()->GetView());
      set_browser_view(nullptr);
    }
  }
  set_content_view(view);
  focused_view_ = view;
  root_view_->AddChildView(content_view());
  root_view_->Layout();
}

void NativeWindowViews::Close() {
  if (!IsClosable()) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

  widget()->Close();
}

void NativeWindowViews::CloseImmediately() {
  widget()->CloseNow();
}

void NativeWindowViews::Focus(bool focus) {
  // For hidden window focus() should do nothing.
  if (!IsVisible())
    return;

  if (focus) {
    widget()->Activate();
  } else {
    widget()->Deactivate();
  }
}

bool NativeWindowViews::IsFocused() {
  return widget()->IsActive();
}

void NativeWindowViews::Show() {
  if (is_modal() && NativeWindow::parent() &&
      !widget()->native_widget_private()->IsVisible())
    static_cast<NativeWindowViews*>(parent())->IncrementChildModals();

  widget()->native_widget_private()->ShowWithWindowState(GetRestoredState());

  NotifyWindowShow();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowMapped();
#endif
}

void NativeWindowViews::ShowInactive() {
  widget()->ShowInactive();

  NotifyWindowShow();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowMapped();
#endif
}

void NativeWindowViews::Hide() {
  if (is_modal() && NativeWindow::parent())
    static_cast<NativeWindowViews*>(parent())->DecrementChildModals();

  widget()->Hide();

  NotifyWindowHide();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowUnmapped();
#endif
}

bool NativeWindowViews::IsVisible() {
  return widget()->IsVisible();
}

bool NativeWindowViews::IsEnabled() {
#if defined(OS_WIN)
  return ::IsWindowEnabled(GetAcceleratedWidget());
#elif defined(USE_X11)
  return !event_disabler_.get();
#endif
}

void NativeWindowViews::IncrementChildModals() {
  num_modal_children_++;
  SetEnabledInternal(ShouldBeEnabled());
}

void NativeWindowViews::DecrementChildModals() {
  if (num_modal_children_ > 0) {
    num_modal_children_--;
  }
  SetEnabledInternal(ShouldBeEnabled());
}

void NativeWindowViews::SetEnabled(bool enable) {
  if (enable != is_enabled_) {
    is_enabled_ = enable;
    SetEnabledInternal(ShouldBeEnabled());
  }
}

bool NativeWindowViews::ShouldBeEnabled() {
  return is_enabled_ && (num_modal_children_ == 0);
}

void NativeWindowViews::SetEnabledInternal(bool enable) {
  if (enable && IsEnabled()) {
    return;
  } else if (!enable && !IsEnabled()) {
    return;
  }

#if defined(OS_WIN)
  ::EnableWindow(GetAcceleratedWidget(), enable);
#elif defined(USE_X11)
  views::DesktopWindowTreeHostX11* tree_host =
      views::DesktopWindowTreeHostX11::GetHostForXID(GetAcceleratedWidget());
  if (enable) {
    tree_host->RemoveEventRewriter(event_disabler_.get());
    event_disabler_.reset();
  } else {
    event_disabler_.reset(new EventDisabler);
    tree_host->AddEventRewriter(event_disabler_.get());
  }
#endif
}

void NativeWindowViews::Maximize() {
#if defined(OS_WIN)
  // For window without WS_THICKFRAME style, we can not call Maximize().
  if (!(::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_THICKFRAME)) {
    restore_bounds_ = GetBounds();
    auto display =
        display::Screen::GetScreen()->GetDisplayNearestPoint(GetPosition());
    SetBounds(display.work_area(), false);
    return;
  }
#endif

  if (IsVisible())
    widget()->Maximize();
  else
    widget()->native_widget_private()->ShowWithWindowState(
        ui::SHOW_STATE_MAXIMIZED);
}

void NativeWindowViews::Unmaximize() {
#if defined(OS_WIN)
  if (!(::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_THICKFRAME)) {
    SetBounds(restore_bounds_, false);
    return;
  }
#endif

  widget()->Restore();
}

bool NativeWindowViews::IsMaximized() {
  return widget()->IsMaximized();
}

void NativeWindowViews::Minimize() {
  if (IsVisible())
    widget()->Minimize();
  else
    widget()->native_widget_private()->ShowWithWindowState(
        ui::SHOW_STATE_MINIMIZED);
}

void NativeWindowViews::Restore() {
  widget()->Restore();
}

bool NativeWindowViews::IsMinimized() {
  return widget()->IsMinimized();
}

void NativeWindowViews::SetFullScreen(bool fullscreen) {
  if (!IsFullScreenable())
    return;

#if defined(OS_WIN)
  // There is no native fullscreen state on Windows.
  bool leaving_fullscreen = IsFullscreen() && !fullscreen;

  if (fullscreen) {
    last_window_state_ = ui::SHOW_STATE_FULLSCREEN;
    NotifyWindowEnterFullScreen();
  } else {
    last_window_state_ = ui::SHOW_STATE_NORMAL;
    NotifyWindowLeaveFullScreen();
  }

  // For window without WS_THICKFRAME style, we can not call SetFullscreen().
  // This path will be used for transparent windows as well.
  if (!thick_frame_) {
    if (fullscreen) {
      restore_bounds_ = GetBounds();
      auto display =
          display::Screen::GetScreen()->GetDisplayNearestPoint(GetPosition());
      SetBounds(display.bounds(), false);
    } else {
      SetBounds(restore_bounds_, false);
    }
    return;
  }

  // We set the new value after notifying, so we can handle the size event
  // correctly.
  widget()->SetFullscreen(fullscreen);

  // If restoring from fullscreen and the window isn't visible, force visible,
  // else a non-responsive window shell could be rendered.
  // (this situation may arise when app starts with fullscreen: true)
  // Note: the following must be after "widget()->SetFullscreen(fullscreen);"
  if (leaving_fullscreen && !IsVisible())
    FlipWindowStyle(GetAcceleratedWidget(), true, WS_VISIBLE);
#else
  if (IsVisible())
    widget()->SetFullscreen(fullscreen);
  else if (fullscreen)
    widget()->native_widget_private()->ShowWithWindowState(
        ui::SHOW_STATE_FULLSCREEN);

  // Auto-hide menubar when in fullscreen.
  if (fullscreen)
    SetMenuBarVisibility(false);
  else
    SetMenuBarVisibility(!IsMenuBarAutoHide());
#endif
}

bool NativeWindowViews::IsFullscreen() const {
  return widget()->IsFullscreen();
}

void NativeWindowViews::SetBounds(const gfx::Rect& bounds, bool animate) {
#if defined(OS_WIN) || defined(USE_X11)
  // On Linux and Windows the minimum and maximum size should be updated with
  // window size when window is not resizable.
  if (!resizable_) {
    SetMaximumSize(bounds.size());
    SetMinimumSize(bounds.size());
  }
#endif

  widget()->SetBounds(bounds);
}

gfx::Rect NativeWindowViews::GetBounds() {
#if defined(OS_WIN)
  if (IsMinimized())
    return widget()->GetRestoredBounds();
#endif

  return widget()->GetWindowBoundsInScreen();
}

gfx::Rect NativeWindowViews::GetContentBounds() {
  return content_view() ? content_view()->GetBoundsInScreen() : gfx::Rect();
}

gfx::Size NativeWindowViews::GetContentSize() {
#if defined(OS_WIN)
  if (IsMinimized())
    return NativeWindow::GetContentSize();
#endif

  return content_view() ? content_view()->size() : gfx::Size();
}

void NativeWindowViews::SetContentSizeConstraints(
    const extensions::SizeConstraints& size_constraints) {
  NativeWindow::SetContentSizeConstraints(size_constraints);
#if defined(OS_WIN)
  // Changing size constraints would force adding the WS_THICKFRAME style, so
  // do nothing if thickFrame is false.
  if (!thick_frame_)
    return;
#endif
  // widget_delegate() is only available after Init() is called, we make use of
  // this to determine whether native widget has initialized.
  if (widget() && widget()->widget_delegate())
    widget()->OnSizeConstraintsChanged();
  if (resizable_)
    old_size_constraints_ = size_constraints;
}

void NativeWindowViews::SetResizable(bool resizable) {
  if (resizable != resizable_) {
    // On Linux there is no "resizable" property of a window, we have to set
    // both the minimum and maximum size to the window size to achieve it.
    if (resizable) {
      SetContentSizeConstraints(old_size_constraints_);
      SetMaximizable(maximizable_);
    } else {
      old_size_constraints_ = GetContentSizeConstraints();
      resizable_ = false;
      gfx::Size content_size = GetContentSize();
      SetContentSizeConstraints(
          extensions::SizeConstraints(content_size, content_size));
    }
  }
#if defined(OS_WIN)
  if (has_frame() && thick_frame_)
    FlipWindowStyle(GetAcceleratedWidget(), resizable, WS_THICKFRAME);
#endif

  resizable_ = resizable;
}

#if defined(OS_WIN)
void NativeWindowViews::MoveTop() {
  gfx::Point pos = GetPosition();
  gfx::Size size = GetSize();
  ::SetWindowPos(GetAcceleratedWidget(), HWND_TOP, pos.x(), pos.y(),
                 size.width(), size.height(),
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}
#endif

bool NativeWindowViews::IsResizable() {
#if defined(OS_WIN)
  if (has_frame())
    return ::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_THICKFRAME;
#endif
  return CanResize();
}

void NativeWindowViews::SetMovable(bool movable) {
  movable_ = movable;
}

bool NativeWindowViews::IsMovable() {
#if defined(OS_WIN)
  return movable_;
#else
  return true;  // Not implemented on Linux.
#endif
}

void NativeWindowViews::SetMinimizable(bool minimizable) {
#if defined(OS_WIN)
  FlipWindowStyle(GetAcceleratedWidget(), minimizable, WS_MINIMIZEBOX);
#endif
  minimizable_ = minimizable;
}

bool NativeWindowViews::IsMinimizable() {
#if defined(OS_WIN)
  return ::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_MINIMIZEBOX;
#else
  return true;  // Not implemented on Linux.
#endif
}

void NativeWindowViews::SetMaximizable(bool maximizable) {
#if defined(OS_WIN)
  FlipWindowStyle(GetAcceleratedWidget(), maximizable, WS_MAXIMIZEBOX);
#endif
  maximizable_ = maximizable;
}

bool NativeWindowViews::IsMaximizable() {
#if defined(OS_WIN)
  return ::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_MAXIMIZEBOX;
#else
  return true;  // Not implemented on Linux.
#endif
}

void NativeWindowViews::SetFullScreenable(bool fullscreenable) {
  fullscreenable_ = fullscreenable;
}

bool NativeWindowViews::IsFullScreenable() {
  return fullscreenable_;
}

void NativeWindowViews::SetClosable(bool closable) {
#if defined(OS_WIN)
  HMENU menu = GetSystemMenu(GetAcceleratedWidget(), false);
  if (closable) {
    EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
  } else {
    EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
  }
#endif
}

bool NativeWindowViews::IsClosable() {
#if defined(OS_WIN)
  HMENU menu = GetSystemMenu(GetAcceleratedWidget(), false);
  MENUITEMINFO info;
  memset(&info, 0, sizeof(info));
  info.cbSize = sizeof(info);
  info.fMask = MIIM_STATE;
  if (!GetMenuItemInfo(menu, SC_CLOSE, false, &info)) {
    return false;
  }
  return !(info.fState & MFS_DISABLED);
#elif defined(USE_X11)
  return true;
#endif
}

void NativeWindowViews::SetAlwaysOnTop(bool top,
                                       const std::string& level,
                                       int relativeLevel,
                                       std::string* error) {
  widget()->SetAlwaysOnTop(top);
}

bool NativeWindowViews::IsAlwaysOnTop() {
  return widget()->IsAlwaysOnTop();
}

void NativeWindowViews::Center() {
  widget()->CenterWindow(GetSize());
}

void NativeWindowViews::Invalidate() {
  widget()->SchedulePaintInRect(gfx::Rect(GetBounds().size()));
}

void NativeWindowViews::SetTitle(const std::string& title) {
  title_ = title;
  widget()->UpdateWindowTitle();
}

std::string NativeWindowViews::GetTitle() {
  return title_;
}

void NativeWindowViews::FlashFrame(bool flash) {
#if defined(OS_WIN)
  // The Chromium's implementation has a bug stopping flash.
  if (!flash) {
    FLASHWINFO fwi;
    fwi.cbSize = sizeof(fwi);
    fwi.hwnd = GetAcceleratedWidget();
    fwi.dwFlags = FLASHW_STOP;
    fwi.uCount = 0;
    FlashWindowEx(&fwi);
    return;
  }
#endif
  widget()->FlashFrame(flash);
}

void NativeWindowViews::SetSkipTaskbar(bool skip) {
#if defined(OS_WIN)
  Microsoft::WRL::ComPtr<ITaskbarList> taskbar;
  if (FAILED(::CoCreateInstance(CLSID_TaskbarList, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&taskbar))) ||
      FAILED(taskbar->HrInit()))
    return;
  if (skip) {
    taskbar->DeleteTab(GetAcceleratedWidget());
  } else {
    taskbar->AddTab(GetAcceleratedWidget());
    taskbar_host_.RestoreThumbarButtons(GetAcceleratedWidget());
  }
#elif defined(USE_X11)
  SetWMSpecState(GetAcceleratedWidget(), skip,
                 GetAtom("_NET_WM_STATE_SKIP_TASKBAR"));
#endif
}

void NativeWindowViews::SetSimpleFullScreen(bool simple_fullscreen) {
  SetFullScreen(simple_fullscreen);
}

bool NativeWindowViews::IsSimpleFullScreen() {
  return IsFullscreen();
}

void NativeWindowViews::SetKiosk(bool kiosk) {
  SetFullScreen(kiosk);
}

bool NativeWindowViews::IsKiosk() {
  return IsFullscreen();
}

void NativeWindowViews::SetBackgroundColor(SkColor background_color) {
  // web views' background color.
  root_view_->SetBackground(views::CreateSolidBackground(background_color));

#if defined(OS_WIN)
  // Set the background color of native window.
  HBRUSH brush = CreateSolidBrush(skia::SkColorToCOLORREF(background_color));
  ULONG_PTR previous_brush =
      SetClassLongPtr(GetAcceleratedWidget(), GCLP_HBRBACKGROUND,
                      reinterpret_cast<LONG_PTR>(brush));
  if (previous_brush)
    DeleteObject((HBRUSH)previous_brush);
  InvalidateRect(GetAcceleratedWidget(), NULL, 1);
#endif
}

void NativeWindowViews::SetHasShadow(bool has_shadow) {
  wm::SetShadowElevation(GetNativeWindow(),
                         has_shadow ? wm::kShadowElevationInactiveWindow
                                    : wm::kShadowElevationNone);
}

bool NativeWindowViews::HasShadow() {
  return GetNativeWindow()->GetProperty(wm::kShadowElevationKey) !=
         wm::kShadowElevationNone;
}

void NativeWindowViews::SetOpacity(const double opacity) {
#if defined(OS_WIN)
  HWND hwnd = GetAcceleratedWidget();
  if (!layered_) {
    LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
    ex_style |= WS_EX_LAYERED;
    ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    layered_ = true;
  }
  ::SetLayeredWindowAttributes(hwnd, 0, opacity * 255, LWA_ALPHA);
#endif
  opacity_ = opacity;
}

double NativeWindowViews::GetOpacity() {
  return opacity_;
}

void NativeWindowViews::SetIgnoreMouseEvents(bool ignore, bool forward) {
#if defined(OS_WIN)
  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  if (ignore)
    ex_style |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
  else
    ex_style &= ~(WS_EX_TRANSPARENT | WS_EX_LAYERED);
  if (layered_)
    ex_style |= WS_EX_LAYERED;
  ::SetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE, ex_style);

  // Forwarding is always disabled when not ignoring mouse messages.
  if (!ignore) {
    SetForwardMouseMessages(false);
  } else {
    SetForwardMouseMessages(forward);
  }
#elif defined(USE_X11)
  if (ignore) {
    XRectangle r = {0, 0, 1, 1};
    XShapeCombineRectangles(gfx::GetXDisplay(), GetAcceleratedWidget(),
                            ShapeInput, 0, 0, &r, 1, ShapeSet, YXBanded);
  } else {
    XShapeCombineMask(gfx::GetXDisplay(), GetAcceleratedWidget(), ShapeInput, 0,
                      0, x11::None, ShapeSet);
  }
#endif
}

void NativeWindowViews::SetContentProtection(bool enable) {
#if defined(OS_WIN)
  DWORD affinity = enable ? WDA_MONITOR : WDA_NONE;
  ::SetWindowDisplayAffinity(GetAcceleratedWidget(), affinity);
#endif
}

void NativeWindowViews::SetFocusable(bool focusable) {
#if defined(OS_WIN)
  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  if (focusable)
    ex_style &= ~WS_EX_NOACTIVATE;
  else
    ex_style |= WS_EX_NOACTIVATE;
  ::SetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE, ex_style);
  SetSkipTaskbar(!focusable);
  Focus(false);
#endif
}

void NativeWindowViews::SetMenu(AtomMenuModel* menu_model) {
#if defined(USE_X11)
  if (menu_model == nullptr) {
    global_menu_bar_.reset();
    root_view_->UnregisterAcceleratorsWithFocusManager();
  }

  if (!global_menu_bar_ && ShouldUseGlobalMenuBar())
    global_menu_bar_.reset(new GlobalMenuBarX11(this));

  // Use global application menu bar when possible.
  if (global_menu_bar_ && global_menu_bar_->IsServerStarted()) {
    root_view_->RegisterAcceleratorsWithFocusManager(menu_model);
    global_menu_bar_->SetMenu(menu_model);
    return;
  }
#endif

  // Should reset content size when setting menu.
  gfx::Size content_size = GetContentSize();
  bool should_reset_size = use_content_size_ && has_frame() &&
                           !IsMenuBarAutoHide() &&
                           ((!!menu_model) != root_view_->HasMenu());

  root_view_->SetMenu(menu_model);

  if (should_reset_size) {
    // Enlarge the size constraints for the menu.
    int menu_bar_height = root_view_->GetMenuBarHeight();
    extensions::SizeConstraints constraints = GetContentSizeConstraints();
    if (constraints.HasMinimumSize()) {
      gfx::Size min_size = constraints.GetMinimumSize();
      min_size.set_height(min_size.height() + menu_bar_height);
      constraints.set_minimum_size(min_size);
    }
    if (constraints.HasMaximumSize()) {
      gfx::Size max_size = constraints.GetMaximumSize();
      max_size.set_height(max_size.height() + menu_bar_height);
      constraints.set_maximum_size(max_size);
    }
    SetContentSizeConstraints(constraints);

    // Resize the window to make sure content size is not changed.
    SetContentSize(content_size);
  }
}

void NativeWindowViews::SetBrowserView(NativeBrowserView* view) {
  if (!content_view())
    return;

  if (browser_view()) {
    content_view()->RemoveChildView(
        browser_view()->GetInspectableWebContentsView()->GetView());
    set_browser_view(nullptr);
  }

  if (!view) {
    return;
  }

  // Add as child of the main web view to avoid (0, 0) origin from overlapping
  // with menu bar.
  set_browser_view(view);
  content_view()->AddChildView(
      view->GetInspectableWebContentsView()->GetView());
}

void NativeWindowViews::SetParentWindow(NativeWindow* parent) {
  NativeWindow::SetParentWindow(parent);

#if defined(USE_X11)
  XDisplay* xdisplay = gfx::GetXDisplay();
  XSetTransientForHint(
      xdisplay, GetAcceleratedWidget(),
      parent ? parent->GetAcceleratedWidget() : DefaultRootWindow(xdisplay));
#elif defined(OS_WIN) && defined(DEBUG)
  // Should work, but does not, it seems that the views toolkit doesn't support
  // reparenting on desktop.
  if (parent) {
    ::SetParent(GetAcceleratedWidget(), parent->GetAcceleratedWidget());
    views::Widget::ReparentNativeView(GetNativeWindow(),
                                      parent->GetNativeWindow());
    wm::AddTransientChild(parent->GetNativeWindow(), GetNativeWindow());
  } else {
    if (!GetNativeWindow()->parent())
      return;
    ::SetParent(GetAcceleratedWidget(), NULL);
    views::Widget::ReparentNativeView(GetNativeWindow(), nullptr);
    wm::RemoveTransientChild(GetNativeWindow()->parent(), GetNativeWindow());
  }
#endif
}

gfx::NativeView NativeWindowViews::GetNativeView() const {
  return widget()->GetNativeView();
}

gfx::NativeWindow NativeWindowViews::GetNativeWindow() const {
  return widget()->GetNativeWindow();
}

void NativeWindowViews::SetProgressBar(double progress,
                                       NativeWindow::ProgressState state) {
#if defined(OS_WIN)
  taskbar_host_.SetProgressBar(GetAcceleratedWidget(), progress, state);
#elif defined(USE_X11)
  if (unity::IsRunning()) {
    unity::SetProgressFraction(progress);
  }
#endif
}

void NativeWindowViews::SetOverlayIcon(const gfx::Image& overlay,
                                       const std::string& description) {
#if defined(OS_WIN)
  taskbar_host_.SetOverlayIcon(GetAcceleratedWidget(), overlay, description);
#endif
}

void NativeWindowViews::SetAutoHideMenuBar(bool auto_hide) {
  root_view_->SetAutoHideMenuBar(auto_hide);
}

bool NativeWindowViews::IsMenuBarAutoHide() {
  return root_view_->IsMenuBarAutoHide();
}

void NativeWindowViews::SetMenuBarVisibility(bool visible) {
  root_view_->SetMenuBarVisibility(visible);
}

bool NativeWindowViews::IsMenuBarVisible() {
  return root_view_->IsMenuBarVisible();
}

void NativeWindowViews::SetVisibleOnAllWorkspaces(bool visible) {
  widget()->SetVisibleOnAllWorkspaces(visible);
}

bool NativeWindowViews::IsVisibleOnAllWorkspaces() {
#if defined(USE_X11)
  // Use the presence/absence of _NET_WM_STATE_STICKY in _NET_WM_STATE to
  // determine whether the current window is visible on all workspaces.
  XAtom sticky_atom = GetAtom("_NET_WM_STATE_STICKY");
  std::vector<XAtom> wm_states;
  ui::GetAtomArrayProperty(GetAcceleratedWidget(), "_NET_WM_STATE", &wm_states);
  return std::find(wm_states.begin(), wm_states.end(), sticky_atom) !=
         wm_states.end();
#endif
  return false;
}

gfx::AcceleratedWidget NativeWindowViews::GetAcceleratedWidget() const {
  return GetNativeWindow()->GetHost()->GetAcceleratedWidget();
}

NativeWindowHandle NativeWindowViews::GetNativeWindowHandle() const {
  return GetAcceleratedWidget();
}

gfx::Rect NativeWindowViews::ContentBoundsToWindowBounds(
    const gfx::Rect& bounds) const {
  if (!has_frame())
    return bounds;

  gfx::Rect window_bounds(bounds);
#if defined(OS_WIN)
  HWND hwnd = GetAcceleratedWidget();
  gfx::Rect dpi_bounds = display::win::ScreenWin::DIPToScreenRect(hwnd, bounds);
  window_bounds = display::win::ScreenWin::ScreenToDIPRect(
      hwnd,
      widget()->non_client_view()->GetWindowBoundsForClientBounds(dpi_bounds));
#endif

  if (root_view_->HasMenu() && root_view_->IsMenuBarVisible()) {
    int menu_bar_height = root_view_->GetMenuBarHeight();
    window_bounds.set_y(window_bounds.y() - menu_bar_height);
    window_bounds.set_height(window_bounds.height() + menu_bar_height);
  }
  return window_bounds;
}

gfx::Rect NativeWindowViews::WindowBoundsToContentBounds(
    const gfx::Rect& bounds) const {
  if (!has_frame())
    return bounds;

  gfx::Rect content_bounds(bounds);
#if defined(OS_WIN)
  HWND hwnd = GetAcceleratedWidget();
  content_bounds.set_size(
      display::win::ScreenWin::DIPToScreenSize(hwnd, content_bounds.size()));
  RECT rect;
  SetRectEmpty(&rect);
  DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
  DWORD ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
  AdjustWindowRectEx(&rect, style, FALSE, ex_style);
  content_bounds.set_width(content_bounds.width() - (rect.right - rect.left));
  content_bounds.set_height(content_bounds.height() - (rect.bottom - rect.top));
  content_bounds.set_size(
      display::win::ScreenWin::ScreenToDIPSize(hwnd, content_bounds.size()));
#endif

  if (root_view_->HasMenu() && root_view_->IsMenuBarVisible()) {
    int menu_bar_height = root_view_->GetMenuBarHeight();
    content_bounds.set_y(content_bounds.y() + menu_bar_height);
    content_bounds.set_height(content_bounds.height() - menu_bar_height);
  }
  return content_bounds;
}

void NativeWindowViews::UpdateDraggableRegions(
    std::unique_ptr<SkRegion> region) {
  draggable_region_ = std::move(region);
}

#if defined(OS_WIN)
void NativeWindowViews::SetIcon(HICON window_icon, HICON app_icon) {
  // We are responsible for storing the images.
  window_icon_ = base::win::ScopedHICON(CopyIcon(window_icon));
  app_icon_ = base::win::ScopedHICON(CopyIcon(app_icon));

  HWND hwnd = GetAcceleratedWidget();
  SendMessage(hwnd, WM_SETICON, ICON_SMALL,
              reinterpret_cast<LPARAM>(window_icon_.get()));
  SendMessage(hwnd, WM_SETICON, ICON_BIG,
              reinterpret_cast<LPARAM>(app_icon_.get()));
}
#elif defined(USE_X11)
void NativeWindowViews::SetIcon(const gfx::ImageSkia& icon) {
  auto* tree_host = static_cast<views::DesktopWindowTreeHost*>(
      views::DesktopWindowTreeHostX11::GetHostForXID(GetAcceleratedWidget()));
  tree_host->SetWindowIcons(icon, icon);
}
#endif

void NativeWindowViews::OnWidgetActivationChanged(views::Widget* changed_widget,
                                                  bool active) {
  if (changed_widget != widget())
    return;

  if (active)
    NativeWindow::NotifyWindowFocus();
  else
    NativeWindow::NotifyWindowBlur();

  // Hide menu bar when window is blured.
  if (!active && IsMenuBarAutoHide() && IsMenuBarVisible())
    SetMenuBarVisibility(false);

  root_view_->ResetAltState();
}

void NativeWindowViews::OnWidgetBoundsChanged(views::Widget* changed_widget,
                                              const gfx::Rect& bounds) {
  if (changed_widget != widget())
    return;

  // Note: We intentionally use `GetBounds()` instead of `bounds` to properly
  // handle minimized windows on Windows.
  const auto new_bounds = GetBounds();
  if (widget_size_ != new_bounds.size()) {
    if (browser_view()) {
      const auto flags = static_cast<NativeBrowserViewViews*>(browser_view())
                             ->GetAutoResizeFlags();
      int width_delta = 0;
      int height_delta = 0;
      if (flags & kAutoResizeWidth) {
        width_delta = new_bounds.width() - widget_size_.width();
      }
      if (flags & kAutoResizeHeight) {
        height_delta = new_bounds.height() - widget_size_.height();
      }

      auto* view = browser_view()->GetInspectableWebContentsView()->GetView();
      auto new_view_size = view->size();
      new_view_size.set_width(new_view_size.width() + width_delta);
      new_view_size.set_height(new_view_size.height() + height_delta);
      view->SetSize(new_view_size);
    }

    NotifyWindowResize();
    widget_size_ = new_bounds.size();
  }
}

void NativeWindowViews::DeleteDelegate() {
  if (is_modal() && this->parent()) {
    auto* parent = this->parent();
    // Enable parent window after current window gets closed.
    static_cast<NativeWindowViews*>(parent)->DecrementChildModals();
    // Focus on parent window.
    parent->Focus(true);
  }

  NotifyWindowClosed();
}

views::View* NativeWindowViews::GetInitiallyFocusedView() {
  return focused_view_;
}

bool NativeWindowViews::CanResize() const {
  return resizable_;
}

bool NativeWindowViews::CanMaximize() const {
  return resizable_ && maximizable_;
}

bool NativeWindowViews::CanMinimize() const {
#if defined(OS_WIN)
  return minimizable_;
#elif defined(USE_X11)
  return true;
#endif
}

base::string16 NativeWindowViews::GetWindowTitle() const {
  return base::UTF8ToUTF16(title_);
}

views::View* NativeWindowViews::GetContentsView() {
  return root_view_.get();
}

bool NativeWindowViews::ShouldDescendIntoChildForEventHandling(
    gfx::NativeView child,
    const gfx::Point& location) {
  // App window should claim mouse events that fall within the draggable region.
  if (draggable_region() &&
      draggable_region()->contains(location.x(), location.y()))
    return false;

  // And the events on border for dragging resizable frameless window.
  if (!has_frame() && CanResize()) {
    FramelessView* frame =
        static_cast<FramelessView*>(widget()->non_client_view()->frame_view());
    return frame->ResizingBorderHitTest(location) == HTNOWHERE;
  }

  return true;
}

views::ClientView* NativeWindowViews::CreateClientView(views::Widget* widget) {
  return new NativeWindowClientView(widget, root_view_.get(), this);
}

views::NonClientFrameView* NativeWindowViews::CreateNonClientFrameView(
    views::Widget* widget) {
#if defined(OS_WIN)
  WinFrameView* frame_view = new WinFrameView;
  frame_view->Init(this, widget);
  return frame_view;
#else
  if (has_frame()) {
    return new NativeFrameView(this, widget);
  } else {
    FramelessView* frame_view = new FramelessView;
    frame_view->Init(this, widget);
    return frame_view;
  }
#endif
}

void NativeWindowViews::OnWidgetMove() {
  NotifyWindowMove();
}

void NativeWindowViews::HandleKeyboardEvent(
    content::WebContents*,
    const content::NativeWebKeyboardEvent& event) {
  keyboard_event_handler_->HandleKeyboardEvent(event,
                                               root_view_->GetFocusManager());
  root_view_->HandleKeyEvent(event);
}

ui::WindowShowState NativeWindowViews::GetRestoredState() {
  if (IsMaximized())
    return ui::SHOW_STATE_MAXIMIZED;
  if (IsFullscreen())
    return ui::SHOW_STATE_FULLSCREEN;

  return ui::SHOW_STATE_NORMAL;
}

// static
NativeWindow* NativeWindow::Create(const mate::Dictionary& options,
                                   NativeWindow* parent) {
  return new NativeWindowViews(options, parent);
}

}  // namespace atom
