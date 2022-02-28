// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_window_views.h"

#if defined(OS_WIN)
#include <wrl/client.h>
#endif

#include <memory>
#include <utility>
#include <vector>

#include "base/cxx17_backports.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/desktop_media_id.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/native_browser_view_views.h"
#include "shell/browser/ui/drag_util.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/ui/views/root_view.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/browser/web_view_manager.h"
#include "shell/browser/window_list.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/options_switches.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/background.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/client_view.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_util.h"

#if defined(OS_LINUX)
#include "base/strings/string_util.h"
#include "shell/browser/browser.h"
#include "shell/browser/linux/unity_service.h"
#include "shell/browser/ui/views/frameless_view.h"
#include "shell/browser/ui/views/native_frame_view.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_linux.h"
#include "ui/views/window/native_frame_view.h"

#if defined(USE_X11)
#include "shell/browser/ui/views/global_menu_bar_x11.h"
#include "shell/browser/ui/x/event_disabler.h"
#include "shell/browser/ui/x/window_state_watcher.h"
#include "shell/browser/ui/x/x_window_utils.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/shape.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/xproto.h"
#include "ui/gfx/x/xproto_util.h"
#endif

#if defined(USE_OZONE) || defined(USE_X11)
#include "ui/base/ui_base_features.h"
#endif

#elif defined(OS_WIN)
#include "base/win/win_util.h"
#include "extensions/common/image_util.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "shell/browser/ui/win/electron_desktop_native_widget_aura.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/base/win/shell.h"
#include "ui/display/screen.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

namespace electron {

#if defined(OS_WIN)

// Similar to the ones in display::win::ScreenWin, but with rounded values
// These help to avoid problems that arise from unresizable windows where the
// original ceil()-ed values can cause calculation errors, since converting
// both ways goes through a ceil() call. Related issue: #15816
gfx::Rect ScreenToDIPRect(HWND hwnd, const gfx::Rect& pixel_bounds) {
  float scale_factor = display::win::ScreenWin::GetScaleFactorForHWND(hwnd);
  gfx::Rect dip_rect = ScaleToRoundedRect(pixel_bounds, 1.0f / scale_factor);
  dip_rect.set_origin(
      display::win::ScreenWin::ScreenToDIPRect(hwnd, pixel_bounds).origin());
  return dip_rect;
}

#endif

namespace {

#if defined(OS_WIN)
const LPCWSTR kUniqueTaskBarClassName = L"Shell_TrayWnd";

void FlipWindowStyle(HWND handle, bool on, DWORD flag) {
  DWORD style = ::GetWindowLong(handle, GWL_STYLE);
  if (on)
    style |= flag;
  else
    style &= ~flag;
  ::SetWindowLong(handle, GWL_STYLE, style);
  // Window's frame styles are cached so we need to call SetWindowPos
  // with the SWP_FRAMECHANGED flag to update cache properly.
  ::SetWindowPos(handle, 0, 0, 0, 0, 0,  // ignored
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

gfx::Rect DIPToScreenRect(HWND hwnd, const gfx::Rect& pixel_bounds) {
  float scale_factor = display::win::ScreenWin::GetScaleFactorForHWND(hwnd);
  gfx::Rect screen_rect = ScaleToRoundedRect(pixel_bounds, scale_factor);
  screen_rect.set_origin(
      display::win::ScreenWin::DIPToScreenRect(hwnd, pixel_bounds).origin());
  return screen_rect;
}

#endif

class NativeWindowClientView : public views::ClientView {
 public:
  NativeWindowClientView(views::Widget* widget,
                         views::View* root_view,
                         NativeWindowViews* window)
      : views::ClientView(widget, root_view), window_(window) {}
  ~NativeWindowClientView() override = default;

  views::CloseRequestResult OnWindowCloseRequested() override {
    window_->NotifyWindowCloseButtonClicked();
    return views::CloseRequestResult::kCannotClose;
  }

 private:
  NativeWindowViews* window_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowClientView);
};

}  // namespace

NativeWindowViews::NativeWindowViews(const gin_helper::Dictionary& options,
                                     NativeWindow* parent)
    : NativeWindow(options, parent),
      root_view_(std::make_unique<RootView>(this)),
      keyboard_event_handler_(
          std::make_unique<views::UnhandledKeyboardEventHandler>()) {
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

  overlay_button_color_ = color_utils::GetSysSkColor(COLOR_BTNFACE);
  overlay_symbol_color_ = color_utils::GetSysSkColor(COLOR_BTNTEXT);

  v8::Local<v8::Value> titlebar_overlay;
  if (options.Get(options::ktitleBarOverlay, &titlebar_overlay) &&
      titlebar_overlay->IsObject()) {
    gin_helper::Dictionary titlebar_overlay_obj =
        gin::Dictionary::CreateEmpty(options.isolate());
    options.Get(options::ktitleBarOverlay, &titlebar_overlay_obj);

    std::string overlay_color_string;
    if (titlebar_overlay_obj.Get(options::kOverlayButtonColor,
                                 &overlay_color_string)) {
      bool success = extensions::image_util::ParseCssColorString(
          overlay_color_string, &overlay_button_color_);
      DCHECK(success);
    }

    std::string overlay_symbol_color_string;
    if (titlebar_overlay_obj.Get(options::kOverlaySymbolColor,
                                 &overlay_symbol_color_string)) {
      bool success = extensions::image_util::ParseCssColorString(
          overlay_symbol_color_string, &overlay_symbol_color_);
      DCHECK(success);
    }
  }

  if (title_bar_style_ != TitleBarStyle::kNormal)
    set_has_frame(false);
#endif

  if (enable_larger_than_screen())
    // We need to set a default maximum window size here otherwise Windows
    // will not allow us to resize the window larger than scree.
    // Setting directly to INT_MAX somehow doesn't work, so we just divide
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
    params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;

  // The given window is most likely not rectangular since it uses
  // transparency and has no standard frame, don't show a shadow for it.
  if (transparent() && !has_frame())
    params.shadow_type = views::Widget::InitParams::ShadowType::kNone;

  bool focusable;
  if (options.Get(options::kFocusable, &focusable) && !focusable)
    params.activatable = views::Widget::InitParams::Activatable::kNo;

#if defined(OS_WIN)
  if (parent)
    params.parent = parent->GetNativeWindow();

  params.native_widget = new ElectronDesktopNativeWidgetAura(this);
#elif defined(OS_LINUX)
  std::string name = Browser::Get()->GetName();
  // Set WM_WINDOW_ROLE.
  params.wm_role_name = "browser-window";
  // Set WM_CLASS.
  params.wm_class_name = base::ToLowerASCII(name);
  params.wm_class_class = name;
#endif

  widget()->Init(std::move(params));
  SetCanResize(resizable_);

  bool fullscreen = false;
  options.Get(options::kFullscreen, &fullscreen);

  std::string window_type;
  options.Get(options::kType, &window_type);

#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    // Start monitoring window states.
    window_state_watcher_ = std::make_unique<WindowStateWatcher>(this);

    // Set _GTK_THEME_VARIANT to dark if we have "dark-theme" option set.
    bool use_dark_theme = false;
    if (options.Get(options::kDarkTheme, &use_dark_theme) && use_dark_theme) {
      SetGTKDarkThemeEnabled(use_dark_theme);
    }
  }
#endif

#if defined(OS_LINUX)
  if (parent)
    SetParentWindow(parent);
#endif

#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    // Before the window is mapped the SetWMSpecState can not work, so we have
    // to manually set the _NET_WM_STATE.
    std::vector<x11::Atom> state_atom_list;
    bool skip_taskbar = false;
    if (options.Get(options::kSkipTaskbar, &skip_taskbar) && skip_taskbar) {
      state_atom_list.push_back(x11::GetAtom("_NET_WM_STATE_SKIP_TASKBAR"));
    }

    // Before the window is mapped, there is no SHOW_FULLSCREEN_STATE.
    if (fullscreen) {
      state_atom_list.push_back(x11::GetAtom("_NET_WM_STATE_FULLSCREEN"));
    }

    if (parent) {
      // Force using dialog type for child window.
      window_type = "dialog";

      // Modal window needs the _NET_WM_STATE_MODAL hint.
      if (is_modal())
        state_atom_list.push_back(x11::GetAtom("_NET_WM_STATE_MODAL"));
    }

    if (!state_atom_list.empty())
      SetArrayProperty(static_cast<x11::Window>(GetAcceleratedWidget()),
                       x11::GetAtom("_NET_WM_STATE"), x11::Atom::ATOM,
                       state_atom_list);

    // Set the _NET_WM_WINDOW_TYPE.
    if (!window_type.empty())
      SetWindowType(static_cast<x11::Window>(GetAcceleratedWidget()),
                    window_type);
  }
#endif

#if defined(OS_WIN)
  if (!has_frame()) {
    // Set Window style so that we get a minimize and maximize animation when
    // frameless.
    DWORD frame_style = WS_CAPTION | WS_OVERLAPPED;
    if (resizable_ && thick_frame_)
      frame_style |= WS_THICKFRAME;
    if (minimizable_)
      frame_style |= WS_MINIMIZEBOX;
    if (maximizable_)
      frame_style |= WS_MAXIMIZEBOX;
    if (!thick_frame_ || !has_frame())
      frame_style &= ~WS_CAPTION;
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
    widget()->set_frame_type(views::Widget::FrameType::kForceNative);
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
#endif

  // Listen to mouse events.
  aura::Window* window = GetNativeWindow();
  if (window)
    window->AddPreTargetHandler(this);

#if defined(OS_LINUX)
  // On linux after the widget is initialized we might have to force set the
  // bounds if the bounds are smaller than the current display
  SetBounds(gfx::Rect(GetPosition(), bounds.size()), false);
#endif

  SetOwnedByWidget(false);
  RegisterDeleteDelegateCallback(base::BindOnce(
      [](NativeWindowViews* window) {
        if (window->is_modal() && window->parent()) {
          auto* parent = window->parent();
          // Enable parent window after current window gets closed.
          static_cast<NativeWindowViews*>(parent)->DecrementChildModals();
          // Focus on parent window.
          parent->Focus(true);
        }

        window->NotifyWindowClosed();
      },
      this));
}

NativeWindowViews::~NativeWindowViews() {
  widget()->RemoveObserver(this);

#if defined(OS_WIN)
  // Disable mouse forwarding to relinquish resources, should any be held.
  SetForwardMouseMessages(false);
#endif

  aura::Window* window = GetNativeWindow();
  if (window)
    window->RemovePreTargetHandler(this);
}

void NativeWindowViews::SetGTKDarkThemeEnabled(bool use_dark_theme) {
#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    const std::string color = use_dark_theme ? "dark" : "light";
    x11::SetStringProperty(static_cast<x11::Window>(GetAcceleratedWidget()),
                           x11::GetAtom("_GTK_THEME_VARIANT"),
                           x11::GetAtom("UTF8_STRING"), color);
  }
#endif
}

void NativeWindowViews::SetContentView(views::View* view) {
  if (content_view()) {
    root_view_->RemoveChildView(content_view());
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

  widget()->native_widget_private()->Show(GetRestoredState(), gfx::Rect());

  // explicitly focus the window
  widget()->Activate();

  NotifyWindowShow();

#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform() && global_menu_bar_)
    global_menu_bar_->OnWindowMapped();
#endif
}

void NativeWindowViews::ShowInactive() {
  widget()->ShowInactive();

  NotifyWindowShow();

#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform() && global_menu_bar_)
    global_menu_bar_->OnWindowMapped();
#endif
}

void NativeWindowViews::Hide() {
  if (is_modal() && NativeWindow::parent())
    static_cast<NativeWindowViews*>(parent())->DecrementChildModals();

  widget()->Hide();

  NotifyWindowHide();

#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform() && global_menu_bar_)
    global_menu_bar_->OnWindowUnmapped();
#endif

#if defined(OS_WIN)
  // When the window is removed from the taskbar via win.hide(),
  // the thumbnail buttons need to be set up again.
  // Ensure that when the window is hidden,
  // the taskbar host is notified that it should re-add them.
  taskbar_host_.SetThumbarButtonsAdded(false);
#endif
}

bool NativeWindowViews::IsVisible() {
  return widget()->IsVisible();
}

bool NativeWindowViews::IsEnabled() {
#if defined(OS_WIN)
  return ::IsWindowEnabled(GetAcceleratedWidget());
#elif defined(OS_LINUX)
#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    return !event_disabler_.get();
  }
#endif
  NOTIMPLEMENTED();
  return true;
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
  if (!features::IsUsingOzonePlatform()) {
    views::DesktopWindowTreeHostPlatform* tree_host =
        views::DesktopWindowTreeHostLinux::GetHostForWidget(
            GetAcceleratedWidget());
    if (enable) {
      tree_host->RemoveEventRewriter(event_disabler_.get());
      event_disabler_.reset();
    } else {
      event_disabler_ = std::make_unique<EventDisabler>();
      tree_host->AddEventRewriter(event_disabler_.get());
    }
  }
#endif
}

#if defined(OS_LINUX)
void NativeWindowViews::Maximize() {
  if (IsVisible())
    widget()->Maximize();
  else
    widget()->native_widget_private()->Show(ui::SHOW_STATE_MAXIMIZED,
                                            gfx::Rect());
}
#endif

void NativeWindowViews::Unmaximize() {
  if (IsMaximized()) {
#if defined(OS_WIN)
    if (transparent()) {
      SetBounds(restore_bounds_, false);
      NotifyWindowUnmaximize();
      return;
    }
#endif

    widget()->Restore();
  }
}

bool NativeWindowViews::IsMaximized() {
  if (widget()->IsMaximized()) {
    return true;
  } else {
#if defined(OS_WIN)
    if (transparent()) {
      // Compare the size of the window with the size of the display
      auto display = display::Screen::GetScreen()->GetDisplayNearestWindow(
          GetNativeWindow());
      // Maximized if the window is the same dimensions and placement as the
      // display
      return GetBounds() == display.work_area();
    }
#endif

    return false;
  }
}

void NativeWindowViews::Minimize() {
  if (IsVisible())
    widget()->Minimize();
  else
    widget()->native_widget_private()->Show(ui::SHOW_STATE_MINIMIZED,
                                            gfx::Rect());
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
    widget()->native_widget_private()->Show(ui::SHOW_STATE_FULLSCREEN,
                                            gfx::Rect());

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
#if defined(OS_WIN) || defined(OS_LINUX)
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

gfx::Rect NativeWindowViews::GetNormalBounds() {
  return widget()->GetRestoredBounds();
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
  SetCanResize(resizable_);
}

bool NativeWindowViews::MoveAbove(const std::string& sourceId) {
  const content::DesktopMediaID id = content::DesktopMediaID::Parse(sourceId);
  if (id.type != content::DesktopMediaID::TYPE_WINDOW)
    return false;

#if defined(OS_WIN)
  const HWND otherWindow = reinterpret_cast<HWND>(id.id);
  if (!::IsWindow(otherWindow))
    return false;

  ::SetWindowPos(GetAcceleratedWidget(), GetWindow(otherWindow, GW_HWNDPREV), 0,
                 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#elif defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    if (!IsWindowValid(static_cast<x11::Window>(id.id)))
      return false;

    electron::MoveWindowAbove(static_cast<x11::Window>(GetAcceleratedWidget()),
                              static_cast<x11::Window>(id.id));
  }
#endif

  return true;
}

void NativeWindowViews::MoveTop() {
// TODO(julien.isorce): fix chromium in order to use existing
// widget()->StackAtTop().
#if defined(OS_WIN)
  gfx::Point pos = GetPosition();
  gfx::Size size = GetSize();
  ::SetWindowPos(GetAcceleratedWidget(), HWND_TOP, pos.x(), pos.y(),
                 size.width(), size.height(),
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#elif defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    electron::MoveWindowToForeground(
        static_cast<x11::Window>(GetAcceleratedWidget()));
  }
#endif
}

bool NativeWindowViews::IsResizable() {
#if defined(OS_WIN)
  if (has_frame())
    return ::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_THICKFRAME;
#endif
  return resizable_;
}

void NativeWindowViews::SetAspectRatio(double aspect_ratio,
                                       const gfx::Size& extra_size) {
  NativeWindow::SetAspectRatio(aspect_ratio, extra_size);
  gfx::SizeF aspect(aspect_ratio, 1.0);
  // Scale up because SetAspectRatio() truncates aspect value to int
  aspect.Scale(100);

  widget()->SetAspectRatio(aspect);
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

void NativeWindowViews::SetExcludedFromShownWindowsMenu(bool excluded) {}

bool NativeWindowViews::IsExcludedFromShownWindowsMenu() {
  // return false on unsupported platforms
  return false;
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
#elif defined(OS_LINUX)
  return true;
#endif
}

void NativeWindowViews::SetAlwaysOnTop(ui::ZOrderLevel z_order,
                                       const std::string& level,
                                       int relativeLevel) {
  bool level_changed = z_order != widget()->GetZOrderLevel();
  widget()->SetZOrderLevel(z_order);

#if defined(OS_WIN)
  // Reset the placement flag.
  behind_task_bar_ = false;
  if (z_order != ui::ZOrderLevel::kNormal) {
    // On macOS the window is placed behind the Dock for the following levels.
    // Re-use the same names on Windows to make it easier for the user.
    static const std::vector<std::string> levels = {
        "floating", "torn-off-menu", "modal-panel", "main-menu", "status"};
    behind_task_bar_ = base::Contains(levels, level);
  }
#endif
  MoveBehindTaskBarIfNeeded();

  // This must be notified at the very end or IsAlwaysOnTop
  // will not yet have been updated to reflect the new status
  if (level_changed)
    NativeWindow::NotifyWindowAlwaysOnTopChanged();
}

ui::ZOrderLevel NativeWindowViews::GetZOrderLevel() {
  return widget()->GetZOrderLevel();
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
  if (!features::IsUsingOzonePlatform()) {
    SetWMSpecState(static_cast<x11::Window>(GetAcceleratedWidget()), skip,
                   x11::GetAtom("_NET_WM_STATE_SKIP_TASKBAR"));
  }
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

bool NativeWindowViews::IsTabletMode() const {
#if defined(OS_WIN)
  return base::win::IsWindows10TabletMode(GetAcceleratedWidget());
#else
  return false;
#endif
}

SkColor NativeWindowViews::GetBackgroundColor() {
  auto* background = root_view_->background();
  if (!background)
    return SK_ColorTRANSPARENT;
  return background->get_color();
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
  const double boundedOpacity = base::clamp(opacity, 0.0, 1.0);
  HWND hwnd = GetAcceleratedWidget();
  if (!layered_) {
    LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
    ex_style |= WS_EX_LAYERED;
    ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    layered_ = true;
  }
  ::SetLayeredWindowAttributes(hwnd, 0, boundedOpacity * 255, LWA_ALPHA);
  opacity_ = boundedOpacity;
#else
  opacity_ = 1.0;  // setOpacity unsupported on Linux
#endif
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
  if (!features::IsUsingOzonePlatform()) {
    auto* connection = x11::Connection::Get();
    if (ignore) {
      x11::Rectangle r{0, 0, 1, 1};
      connection->shape().Rectangles({
          .operation = x11::Shape::So::Set,
          .destination_kind = x11::Shape::Sk::Input,
          .ordering = x11::ClipOrdering::YXBanded,
          .destination_window =
              static_cast<x11::Window>(GetAcceleratedWidget()),
          .rectangles = {r},
      });
    } else {
      connection->shape().Mask({
          .operation = x11::Shape::So::Set,
          .destination_kind = x11::Shape::Sk::Input,
          .destination_window =
              static_cast<x11::Window>(GetAcceleratedWidget()),
          .source_bitmap = x11::Pixmap::None,
      });
    }
  }
#endif
}

void NativeWindowViews::SetContentProtection(bool enable) {
#if defined(OS_WIN)
  HWND hwnd = GetAcceleratedWidget();
  DWORD affinity = enable ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
  ::SetWindowDisplayAffinity(hwnd, affinity);
  if (!layered_) {
    // Workaround to prevent black window on screen capture after hiding and
    // showing the BrowserWindow.
    LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
    ex_style |= WS_EX_LAYERED;
    ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    layered_ = true;
  }
#endif
}

void NativeWindowViews::SetFocusable(bool focusable) {
  widget()->widget_delegate()->SetCanActivate(focusable);
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

bool NativeWindowViews::IsFocusable() {
  bool can_activate = widget()->widget_delegate()->CanActivate();
#if defined(OS_WIN)
  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  bool no_activate = ex_style & WS_EX_NOACTIVATE;
  return !no_activate && can_activate;
#else
  return can_activate;
#endif
}

void NativeWindowViews::SetMenu(ElectronMenuModel* menu_model) {
#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    // Remove global menu bar.
    if (global_menu_bar_ && menu_model == nullptr) {
      global_menu_bar_.reset();
      root_view_->UnregisterAcceleratorsWithFocusManager();
      return;
    }

    // Use global application menu bar when possible.
    if (ShouldUseGlobalMenuBar()) {
      if (!global_menu_bar_)
        global_menu_bar_ = std::make_unique<GlobalMenuBarX11>(this);
      if (global_menu_bar_->IsServerStarted()) {
        root_view_->RegisterAcceleratorsWithFocusManager(menu_model);
        global_menu_bar_->SetMenu(menu_model);
        return;
      }
    }
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

void NativeWindowViews::AddBrowserView(NativeBrowserView* view) {
  if (!content_view())
    return;

  if (!view) {
    return;
  }

  add_browser_view(view);
  if (view->GetInspectableWebContentsView())
    content_view()->AddChildView(
        view->GetInspectableWebContentsView()->GetView());
}

void NativeWindowViews::RemoveBrowserView(NativeBrowserView* view) {
  if (!content_view())
    return;

  if (!view) {
    return;
  }

  if (view->GetInspectableWebContentsView())
    content_view()->RemoveChildView(
        view->GetInspectableWebContentsView()->GetView());
  remove_browser_view(view);
}

void NativeWindowViews::SetTopBrowserView(NativeBrowserView* view) {
  if (!content_view())
    return;

  if (!view) {
    return;
  }

  remove_browser_view(view);
  add_browser_view(view);

  if (view->GetInspectableWebContentsView())
    content_view()->ReorderChildView(
        view->GetInspectableWebContentsView()->GetView(), -1);
}

void NativeWindowViews::SetParentWindow(NativeWindow* parent) {
  NativeWindow::SetParentWindow(parent);

#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    x11::SetProperty(
        static_cast<x11::Window>(GetAcceleratedWidget()),
        x11::Atom::WM_TRANSIENT_FOR, x11::Atom::WINDOW,
        parent ? static_cast<x11::Window>(parent->GetAcceleratedWidget())
               : ui::GetX11RootWindow());
  }
#elif defined(OS_WIN)
  // To set parentship between windows into Windows is better to play with the
  //  owner instead of the parent, as Windows natively seems to do if a parent
  //  is specified at window creation time.
  // For do this we must NOT use the ::SetParent function, instead we must use
  //  the ::GetWindowLongPtr or ::SetWindowLongPtr functions with "nIndex" set
  //  to "GWLP_HWNDPARENT" which actually means the window owner.
  HWND hwndParent = parent ? parent->GetAcceleratedWidget() : NULL;
  if (hwndParent ==
      (HWND)::GetWindowLongPtr(GetAcceleratedWidget(), GWLP_HWNDPARENT))
    return;
  ::SetWindowLongPtr(GetAcceleratedWidget(), GWLP_HWNDPARENT,
                     (LONG_PTR)hwndParent);
  // Ensures the visibility
  if (IsVisible()) {
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    ::GetWindowPlacement(GetAcceleratedWidget(), &wp);
    ::ShowWindow(GetAcceleratedWidget(), SW_HIDE);
    ::ShowWindow(GetAcceleratedWidget(), wp.showCmd);
    ::BringWindowToTop(GetAcceleratedWidget());
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
#elif defined(OS_LINUX)
  if (unity::IsRunning()) {
    unity::SetProgressFraction(progress);
  }
#endif
}

void NativeWindowViews::SetOverlayIcon(const gfx::Image& overlay,
                                       const std::string& description) {
#if defined(OS_WIN)
  SkBitmap overlay_bitmap = overlay.AsBitmap();
  taskbar_host_.SetOverlayIcon(GetAcceleratedWidget(), overlay_bitmap,
                               description);
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

void NativeWindowViews::SetVisibleOnAllWorkspaces(
    bool visible,
    bool visibleOnFullScreen,
    bool skipTransformProcessType) {
  widget()->SetVisibleOnAllWorkspaces(visible);
}

bool NativeWindowViews::IsVisibleOnAllWorkspaces() {
#if defined(USE_X11)
  if (!features::IsUsingOzonePlatform()) {
    // Use the presence/absence of _NET_WM_STATE_STICKY in _NET_WM_STATE to
    // determine whether the current window is visible on all workspaces.
    x11::Atom sticky_atom = x11::GetAtom("_NET_WM_STATE_STICKY");
    std::vector<x11::Atom> wm_states;
    GetArrayProperty(static_cast<x11::Window>(GetAcceleratedWidget()),
                     x11::GetAtom("_NET_WM_STATE"), &wm_states);
    return std::find(wm_states.begin(), wm_states.end(), sticky_atom) !=
           wm_states.end();
  }
#endif
  return false;
}

content::DesktopMediaID NativeWindowViews::GetDesktopMediaID() const {
  const gfx::AcceleratedWidget accelerated_widget = GetAcceleratedWidget();
  content::DesktopMediaID::Id window_handle = content::DesktopMediaID::kNullId;
  content::DesktopMediaID::Id aura_id = content::DesktopMediaID::kNullId;
#if defined(OS_WIN)
  window_handle =
      reinterpret_cast<content::DesktopMediaID::Id>(accelerated_widget);
#elif defined(OS_LINUX)
  window_handle = static_cast<uint32_t>(accelerated_widget);
#endif
  aura::WindowTreeHost* const host =
      aura::WindowTreeHost::GetForAcceleratedWidget(accelerated_widget);
  aura::Window* const aura_window = host ? host->window() : nullptr;
  if (aura_window) {
    aura_id = content::DesktopMediaID::RegisterNativeWindow(
                  content::DesktopMediaID::TYPE_WINDOW, aura_window)
                  .window_id;
  }

  // No constructor to pass the aura_id. Make sure to not use the other
  // constructor that has a third parameter, it is for yet another purpose.
  content::DesktopMediaID result = content::DesktopMediaID(
      content::DesktopMediaID::TYPE_WINDOW, window_handle);

  // Confusing but this is how content::DesktopMediaID is designed. The id
  // property is the window handle whereas the window_id property is an id
  // given by a map containing all aura instances.
  result.window_id = aura_id;
  return result;
}

gfx::AcceleratedWidget NativeWindowViews::GetAcceleratedWidget() const {
  if (GetNativeWindow() && GetNativeWindow()->GetHost())
    return GetNativeWindow()->GetHost()->GetAcceleratedWidget();
  else
    return gfx::kNullAcceleratedWidget;
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
  if (widget()->non_client_view()) {
    HWND hwnd = GetAcceleratedWidget();
    gfx::Rect dpi_bounds = DIPToScreenRect(hwnd, bounds);
    window_bounds = ScreenToDIPRect(
        hwnd, widget()->non_client_view()->GetWindowBoundsForClientBounds(
                  dpi_bounds));
  }
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
  content_bounds.set_size(DIPToScreenRect(hwnd, content_bounds).size());
  RECT rect;
  SetRectEmpty(&rect);
  DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
  DWORD ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
  AdjustWindowRectEx(&rect, style, FALSE, ex_style);
  content_bounds.set_width(content_bounds.width() - (rect.right - rect.left));
  content_bounds.set_height(content_bounds.height() - (rect.bottom - rect.top));
  content_bounds.set_size(ScreenToDIPRect(hwnd, content_bounds).size());
#endif

  if (root_view_->HasMenu() && root_view_->IsMenuBarVisible()) {
    int menu_bar_height = root_view_->GetMenuBarHeight();
    content_bounds.set_y(content_bounds.y() + menu_bar_height);
    content_bounds.set_height(content_bounds.height() - menu_bar_height);
  }
  return content_bounds;
}

void NativeWindowViews::UpdateDraggableRegions(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  draggable_region_ = DraggableRegionsToSkRegion(regions);
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
#elif defined(OS_LINUX)
void NativeWindowViews::SetIcon(const gfx::ImageSkia& icon) {
  auto* tree_host = views::DesktopWindowTreeHostLinux::GetHostForWidget(
      GetAcceleratedWidget());
  tree_host->SetWindowIcons(icon, {});
}
#endif

void NativeWindowViews::OnWidgetActivationChanged(views::Widget* changed_widget,
                                                  bool active) {
  if (changed_widget != widget())
    return;

  if (active) {
    MoveBehindTaskBarIfNeeded();
    NativeWindow::NotifyWindowFocus();
  } else {
    NativeWindow::NotifyWindowBlur();
  }

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
    int width_delta = new_bounds.width() - widget_size_.width();
    int height_delta = new_bounds.height() - widget_size_.height();
    for (NativeBrowserView* item : browser_views()) {
      auto* native_view = static_cast<NativeBrowserViewViews*>(item);
      native_view->SetAutoResizeProportions(widget_size_);
      native_view->AutoResize(new_bounds, width_delta, height_delta);
    }

    NotifyWindowResize();
    widget_size_ = new_bounds.size();
  }
}

void NativeWindowViews::OnWidgetDestroying(views::Widget* widget) {
  aura::Window* window = GetNativeWindow();
  if (window)
    window->RemovePreTargetHandler(this);
}

void NativeWindowViews::OnWidgetDestroyed(views::Widget* changed_widget) {
  widget_destroyed_ = true;
}

views::View* NativeWindowViews::GetInitiallyFocusedView() {
  return focused_view_;
}

bool NativeWindowViews::CanMaximize() const {
  return resizable_ && maximizable_;
}

bool NativeWindowViews::CanMinimize() const {
#if defined(OS_WIN)
  return minimizable_;
#elif defined(OS_LINUX)
  return true;
#endif
}

std::u16string NativeWindowViews::GetWindowTitle() const {
  return base::UTF8ToUTF16(title_);
}

views::View* NativeWindowViews::GetContentsView() {
  return root_view_.get();
}

bool NativeWindowViews::ShouldDescendIntoChildForEventHandling(
    gfx::NativeView child,
    const gfx::Point& location) {
  // App window should claim mouse events that fall within any BrowserViews'
  // draggable region.
  for (auto* view : browser_views()) {
    auto* native_view = static_cast<NativeBrowserViewViews*>(view);
    auto* view_draggable_region = native_view->draggable_region();
    if (view_draggable_region &&
        view_draggable_region->contains(location.x(), location.y()))
      return false;
  }

  // App window should claim mouse events that fall within the draggable region.
  if (draggable_region() &&
      draggable_region()->contains(location.x(), location.y()))
    return false;

  // And the events on border for dragging resizable frameless window.
  if (!has_frame() && resizable_) {
    auto* frame =
        static_cast<FramelessView*>(widget()->non_client_view()->frame_view());
    return frame->ResizingBorderHitTest(location) == HTNOWHERE;
  }

  return true;
}

views::ClientView* NativeWindowViews::CreateClientView(views::Widget* widget) {
  return new NativeWindowClientView(widget, root_view_.get(), this);
}

std::unique_ptr<views::NonClientFrameView>
NativeWindowViews::CreateNonClientFrameView(views::Widget* widget) {
#if defined(OS_WIN)
  auto frame_view = std::make_unique<WinFrameView>();
  frame_view->Init(this, widget);
  return frame_view;
#else
  if (has_frame()) {
    return std::make_unique<NativeFrameView>(this, widget);
  } else {
    auto frame_view = std::make_unique<FramelessView>();
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
  if (widget_destroyed_)
    return;

#if defined(OS_LINUX)
  if (event.windows_key_code == ui::VKEY_BROWSER_BACK)
    NotifyWindowExecuteAppCommand(kBrowserBackward);
  else if (event.windows_key_code == ui::VKEY_BROWSER_FORWARD)
    NotifyWindowExecuteAppCommand(kBrowserForward);
#endif

  keyboard_event_handler_->HandleKeyboardEvent(event,
                                               root_view_->GetFocusManager());
  root_view_->HandleKeyEvent(event);
}

void NativeWindowViews::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() != ui::ET_MOUSE_PRESSED)
    return;

  // Alt+Click should not toggle menu bar.
  root_view_->ResetAltState();

#if defined(OS_LINUX)
  if (event->changed_button_flags() == ui::EF_BACK_MOUSE_BUTTON)
    NotifyWindowExecuteAppCommand(kBrowserBackward);
  else if (event->changed_button_flags() == ui::EF_FORWARD_MOUSE_BUTTON)
    NotifyWindowExecuteAppCommand(kBrowserForward);
#endif
}

ui::WindowShowState NativeWindowViews::GetRestoredState() {
  if (IsMaximized()) {
#if defined(OS_WIN)
    // Only restore Maximized state when window is NOT transparent style
    if (!transparent()) {
      return ui::SHOW_STATE_MAXIMIZED;
    }
#else
    return ui::SHOW_STATE_MAXIMIZED;
#endif
  }

  if (IsFullscreen())
    return ui::SHOW_STATE_FULLSCREEN;

  return ui::SHOW_STATE_NORMAL;
}

void NativeWindowViews::MoveBehindTaskBarIfNeeded() {
#if defined(OS_WIN)
  if (behind_task_bar_) {
    const HWND task_bar_hwnd = ::FindWindow(kUniqueTaskBarClassName, nullptr);
    ::SetWindowPos(GetAcceleratedWidget(), task_bar_hwnd, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }
#endif
  // TODO(julien.isorce): Implement X11 case.
}

// static
NativeWindow* NativeWindow::Create(const gin_helper::Dictionary& options,
                                   NativeWindow* parent) {
  return new NativeWindowViews(options, parent);
}

}  // namespace electron
