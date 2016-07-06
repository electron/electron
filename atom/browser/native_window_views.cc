// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_views.h"

#include <string>
#include <vector>

#include "atom/browser/ui/views/menu_bar.h"
#include "atom/browser/ui/views/menu_layout.h"
#include "atom/browser/window_list.h"
#include "atom/common/color_util.h"
#include "atom/common/draggable_region.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/options_switches.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "native_mate/dictionary.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/image/image.h"
#include "ui/views/background.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/window/client_view.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/core/shadow_types.h"

#if defined(USE_X11)
#include "atom/browser/browser.h"
#include "atom/browser/ui/views/global_menu_bar_x11.h"
#include "atom/browser/ui/views/frameless_view.h"
#include "atom/browser/ui/views/native_frame_view.h"
#include "atom/browser/ui/x/event_disabler.h"
#include "atom/browser/ui/x/window_state_watcher.h"
#include "atom/browser/ui/x/x_window_utils.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/libgtk2ui/unity_service.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_x11.h"
#include "ui/views/window/native_frame_view.h"
#elif defined(OS_WIN)
#include "atom/browser/ui/views/win_frame_view.h"
#include "atom/browser/ui/win/atom_desktop_window_tree_host_win.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/base/win/shell.h"
#include "ui/gfx/win/dpi.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

#include <iostream>
#include "atom/browser/osr_window.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/context_factory.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "ui/gfx/native_widget_types.h"

namespace atom {

namespace {

// The menu bar height in pixels.
#if defined(OS_WIN)
const int kMenuBarHeight = 20;
#else
const int kMenuBarHeight = 25;
#endif

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

bool IsAltKey(const content::NativeWebKeyboardEvent& event) {
  return event.windowsKeyCode == ui::VKEY_MENU;
}

bool IsAltModifier(const content::NativeWebKeyboardEvent& event) {
  typedef content::NativeWebKeyboardEvent::Modifiers Modifiers;
  int modifiers = event.modifiers;
  modifiers &= ~Modifiers::NumLockOn;
  modifiers &= ~Modifiers::CapsLockOn;
  return (modifiers == Modifiers::AltKey) ||
         (modifiers == (Modifiers::AltKey | Modifiers::IsLeft)) ||
         (modifiers == (Modifiers::AltKey | Modifiers::IsRight));
}

#if defined(USE_X11)
int SendClientEvent(XDisplay* display, ::Window window, const char* msg) {
  XEvent event = {};
  event.xclient.type = ClientMessage;
  event.xclient.send_event = True;
  event.xclient.message_type = XInternAtom(display, msg, False);
  event.xclient.window = window;
  event.xclient.format = 32;
  XSendEvent(display, DefaultRootWindow(display), False,
             SubstructureRedirectMask | SubstructureNotifyMask, &event);
  XFlush(display);
  return True;
}
#endif

class NativeWindowClientView : public views::ClientView {
 public:
  NativeWindowClientView(views::Widget* widget,
                         NativeWindowViews* contents_view)
      : views::ClientView(widget, contents_view) {
  }
  virtual ~NativeWindowClientView() {}

  bool CanClose() override {
    static_cast<NativeWindowViews*>(contents_view())->RequestToClosePage();
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeWindowClientView);
};

}  // namespace

NativeWindowViews::NativeWindowViews(
    brightray::InspectableWebContents* web_contents,
    const mate::Dictionary& options,
    NativeWindow* parent)
    : NativeWindow(web_contents, options, parent),
      window_(new views::Widget),
      web_view_(inspectable_web_contents()->GetView()->GetView()),
      menu_bar_autohide_(false),
      menu_bar_visible_(false),
      menu_bar_alt_pressed_(false),
      keyboard_event_handler_(new views::UnhandledKeyboardEventHandler),
      disable_count_(0),
      use_content_size_(false),
      movable_(true),
      resizable_(true),
      maximizable_(true),
      minimizable_(true),
      fullscreenable_(true) {
  options.Get(options::kTitle, &title_);
  options.Get(options::kAutoHideMenuBar, &menu_bar_autohide_);

#if defined(OS_WIN)
  // On Windows we rely on the CanResize() to indicate whether window can be
  // resized, and it should be set before window is created.
  options.Get(options::kResizable, &resizable_);
  options.Get(options::kMinimizable, &minimizable_);
  options.Get(options::kMaximizable, &maximizable_);
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

  window_->AddObserver(this);

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

  params.native_widget =
      new views::DesktopNativeWidgetAura(window_.get());
  atom_desktop_window_tree_host_win_ = new AtomDesktopWindowTreeHostWin(
      this,
      window_.get(),
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

  window_->Init(params);

  bool fullscreen = false;
  options.Get(options::kFullscreen, &fullscreen);

#if defined(USE_X11)
  // Start monitoring window states.
  window_state_watcher_.reset(new WindowStateWatcher(this));

  // Set _GTK_THEME_VARIANT to dark if we have "dark-theme" option set.
  bool use_dark_theme = false;
  if (options.Get(options::kDarkTheme, &use_dark_theme) && use_dark_theme) {
    XDisplay* xdisplay = gfx::GetXDisplay();
    XChangeProperty(xdisplay, GetAcceleratedWidget(),
                    XInternAtom(xdisplay, "_GTK_THEME_VARIANT", False),
                    XInternAtom(xdisplay, "UTF8_STRING", False),
                    8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>("dark"),
                    4);
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

  std::string window_type;
  options.Get(options::kType, &window_type);

  if (parent) {
    SetParentWindow(parent);
    // Force using dialog type for child window.
    window_type = "dialog";
    // Modal window needs the _NET_WM_STATE_MODAL hint.
    if (is_modal())
      state_atom_list.push_back(GetAtom("_NET_WM_STATE_MODAL"));
  }

  ui::SetAtomArrayProperty(GetAcceleratedWidget(), "_NET_WM_STATE", "ATOM",
                           state_atom_list);

  // Set the _NET_WM_WINDOW_TYPE.
  if (!window_type.empty())
    SetWindowType(GetAcceleratedWidget(), window_type);
#endif

  // Add web view.
  SetLayoutManager(new MenuLayout(this, kMenuBarHeight));

  AddChildView(web_view_);

#if defined(OS_WIN)
  // Save initial window state.
  if (fullscreen)
    last_window_state_ = ui::SHOW_STATE_FULLSCREEN;
  else
    last_window_state_ = ui::SHOW_STATE_NORMAL;
  last_normal_size_ = gfx::Size(widget_size_);

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
    if (transparent())
      frame_style &= ~(WS_THICKFRAME | WS_CAPTION);
    ::SetWindowLong(GetAcceleratedWidget(), GWL_STYLE, frame_style);
  }

  if (transparent()) {
    // Transparent window on Windows has to have WS_EX_COMPOSITED style.
    LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
    ex_style |= WS_EX_COMPOSITED;
    ::SetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE, ex_style);
  }
#endif

  // TODO(zcbenz): This was used to force using native frame on Windows 2003, we
  // should check whether setting it in InitParams can work.
  if (has_frame()) {
    window_->set_frame_type(views::Widget::FrameType::FRAME_TYPE_FORCE_NATIVE);
    window_->FrameTypeChanged();
  }

  gfx::Size size = bounds.size();
  if (has_frame() &&
      options.Get(options::kUseContentSize, &use_content_size_) &&
      use_content_size_)
    size = ContentSizeToWindowSize(size);

  window_->CenterWindow(size);
  Layout();
}

NativeWindowViews::~NativeWindowViews() {
  window_->RemoveObserver(this);
}

void NativeWindowViews::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  std::cout << "NativeWindowViews::RenderViewCreated" << std::endl;
  NativeWindow::RenderViewCreated(render_view_host);

  content::RenderWidgetHostImpl* impl = content::RenderWidgetHostImpl::FromID(
      render_view_host->GetProcess()->GetID(),
      render_view_host->GetRoutingID());
  if (impl) {
    ui::Layer* layer = widget()->GetCompositor()->root_layer();
    ui::Compositor* compositor_ = widget()->GetCompositor();
    compositor_->RequestNewOutputSurface();
  }
}

void NativeWindowViews::Close() {
  if (!IsClosable()) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

  window_->Close();
}

void NativeWindowViews::CloseImmediately() {
  window_->CloseNow();
}

void NativeWindowViews::Focus(bool focus) {
  if (focus) {
#if defined(OS_WIN)
    window_->Activate();
#elif defined(USE_X11)
    // The "Activate" implementation of Chromium is not reliable on Linux.
    ::Window window = GetAcceleratedWidget();
    XDisplay* xdisplay = gfx::GetXDisplay();
    SendClientEvent(xdisplay, window, "_NET_ACTIVE_WINDOW");
    XMapRaised(xdisplay, window);
#endif
  } else {
    window_->Deactivate();
  }
}

bool NativeWindowViews::IsFocused() {
  return window_->IsActive();
}

void NativeWindowViews::Show() {
  if (is_modal() && NativeWindow::parent())
    static_cast<NativeWindowViews*>(NativeWindow::parent())->SetEnabled(false);

  window_->native_widget_private()->ShowWithWindowState(GetRestoredState());

  NotifyWindowShow();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowMapped();
#endif
}

void NativeWindowViews::ShowInactive() {
  window_->ShowInactive();

  NotifyWindowShow();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowMapped();
#endif
}

void NativeWindowViews::Hide() {
  if (is_modal() && NativeWindow::parent())
    static_cast<NativeWindowViews*>(NativeWindow::parent())->SetEnabled(true);

  window_->Hide();

  NotifyWindowHide();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowUnmapped();
#endif
}

bool NativeWindowViews::IsVisible() {
  return window_->IsVisible();
}

bool NativeWindowViews::IsEnabled() {
#if defined(OS_WIN)
  return ::IsWindowEnabled(GetAcceleratedWidget());
#elif defined(USE_X11)
  return !event_disabler_.get();
#endif
}

void NativeWindowViews::Maximize() {
  if (IsVisible())
    window_->Maximize();
  else
    window_->native_widget_private()->ShowWithWindowState(
        ui::SHOW_STATE_MAXIMIZED);
}

void NativeWindowViews::Unmaximize() {
  window_->Restore();
}

bool NativeWindowViews::IsMaximized() {
  return window_->IsMaximized();
}

void NativeWindowViews::Minimize() {
  if (IsVisible())
    window_->Minimize();
  else
    window_->native_widget_private()->ShowWithWindowState(
        ui::SHOW_STATE_MINIMIZED);
}

void NativeWindowViews::Restore() {
  window_->Restore();
}

bool NativeWindowViews::IsMinimized() {
  return window_->IsMinimized();
}

void NativeWindowViews::SetFullScreen(bool fullscreen) {
  if (!IsFullScreenable())
    return;

#if defined(OS_WIN)
  // There is no native fullscreen state on Windows.
  if (fullscreen) {
    last_window_state_ = ui::SHOW_STATE_FULLSCREEN;
    NotifyWindowEnterFullScreen();
  } else {
    last_window_state_ = ui::SHOW_STATE_NORMAL;
    NotifyWindowLeaveFullScreen();
  }
  // We set the new value after notifying, so we can handle the size event
  // correctly.
  window_->SetFullscreen(fullscreen);
#else
  if (IsVisible())
    window_->SetFullscreen(fullscreen);
  else
    window_->native_widget_private()->ShowWithWindowState(
        ui::SHOW_STATE_FULLSCREEN);
#endif
}

bool NativeWindowViews::IsFullscreen() const {
  return window_->IsFullscreen();
}

void NativeWindowViews::SetBounds(const gfx::Rect& bounds,
    bool animate = false) {
#if defined(USE_X11)
  // On Linux the minimum and maximum size should be updated with window size
  // when window is not resizable.
  if (!resizable_) {
    SetMaximumSize(bounds.size());
    SetMinimumSize(bounds.size());
  }
#endif

  window_->SetBounds(bounds);
}

gfx::Rect NativeWindowViews::GetBounds() {
#if defined(OS_WIN)
  if (IsMinimized())
    return window_->GetRestoredBounds();
#endif

  return window_->GetWindowBoundsInScreen();
}

gfx::Size NativeWindowViews::GetContentSize() {
#if defined(OS_WIN)
  if (IsMinimized())
    return NativeWindow::GetContentSize();
#endif

  return web_view_->size();
}

void NativeWindowViews::SetContentSizeConstraints(
    const extensions::SizeConstraints& size_constraints) {
  NativeWindow::SetContentSizeConstraints(size_constraints);
  // widget_delegate() is only available after Init() is called, we make use of
  // this to determine whether native widget has initialized.
  if (window_ && window_->widget_delegate())
    window_->OnSizeConstraintsChanged();
#if defined(USE_X11)
  if (resizable_)
    old_size_constraints_ = size_constraints;
#endif
}

void NativeWindowViews::SetResizable(bool resizable) {
#if defined(OS_WIN)
  if (!transparent())
    FlipWindowStyle(GetAcceleratedWidget(), resizable, WS_THICKFRAME);
#elif defined(USE_X11)
  if (resizable != resizable_) {
    // On Linux there is no "resizable" property of a window, we have to set
    // both the minimum and maximum size to the window size to achieve it.
    if (resizable) {
      SetContentSizeConstraints(old_size_constraints_);
    } else {
      old_size_constraints_ = GetContentSizeConstraints();
      resizable_ = false;
      gfx::Size content_size = GetContentSize();
      SetContentSizeConstraints(
          extensions::SizeConstraints(content_size, content_size));
    }
  }
#endif

  resizable_ = resizable;
}

bool NativeWindowViews::IsResizable() {
#if defined(OS_WIN)
  return ::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_THICKFRAME;
#else
  return CanResize();
#endif
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

void NativeWindowViews::SetAlwaysOnTop(bool top) {
  window_->SetAlwaysOnTop(top);
}

bool NativeWindowViews::IsAlwaysOnTop() {
  return window_->IsAlwaysOnTop();
}

void NativeWindowViews::Center() {
  window_->CenterWindow(GetSize());
}

void NativeWindowViews::SetTitle(const std::string& title) {
  title_ = title;
  window_->UpdateWindowTitle();
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
  window_->FlashFrame(flash);
}

void NativeWindowViews::SetSkipTaskbar(bool skip) {
#if defined(OS_WIN)
  base::win::ScopedComPtr<ITaskbarList> taskbar;
  if (FAILED(taskbar.CreateInstance(CLSID_TaskbarList, NULL,
                                    CLSCTX_INPROC_SERVER)) ||
      FAILED(taskbar->HrInit()))
    return;
  if (skip)
    taskbar->DeleteTab(GetAcceleratedWidget());
  else
    taskbar->AddTab(GetAcceleratedWidget());
#elif defined(USE_X11)
  SetWMSpecState(GetAcceleratedWidget(), skip,
                 GetAtom("_NET_WM_STATE_SKIP_TASKBAR"));
#endif
}

void NativeWindowViews::SetKiosk(bool kiosk) {
  SetFullScreen(kiosk);
}

bool NativeWindowViews::IsKiosk() {
  return IsFullscreen();
}

void NativeWindowViews::SetBackgroundColor(const std::string& color_name) {
  // web views' background color.
  SkColor background_color = ParseHexColor(color_name);
  set_background(views::Background::CreateSolidBackground(background_color));

#if defined(OS_WIN)
  // Set the background color of native window.
  HBRUSH brush = CreateSolidBrush(skia::SkColorToCOLORREF(background_color));
  ULONG_PTR previous_brush = SetClassLongPtr(
      GetAcceleratedWidget(),
      GCLP_HBRBACKGROUND,
      reinterpret_cast<LONG_PTR>(brush));
  if (previous_brush)
    DeleteObject((HBRUSH)previous_brush);
#endif
}

void NativeWindowViews::SetHasShadow(bool has_shadow) {
  wm::SetShadowType(
      GetNativeWindow(),
      has_shadow ? wm::SHADOW_TYPE_RECTANGULAR : wm::SHADOW_TYPE_NONE);
}

bool NativeWindowViews::HasShadow() {
  return wm::GetShadowType(GetNativeWindow()) != wm::SHADOW_TYPE_NONE;
}

void NativeWindowViews::SetIgnoreMouseEvents(bool ignore) {
#if defined(OS_WIN)
  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  if (ignore)
    ex_style |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
  else
    ex_style &= ~(WS_EX_TRANSPARENT | WS_EX_LAYERED);
  ::SetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE, ex_style);
#elif defined(USE_X11)
  if (ignore) {
    XRectangle r = {0, 0, 1, 1};
    XShapeCombineRectangles(gfx::GetXDisplay(), GetAcceleratedWidget(),
                            ShapeInput, 0, 0, &r, 1, ShapeSet, YXBanded);
  } else {
    XShapeCombineMask(gfx::GetXDisplay(), GetAcceleratedWidget(),
                      ShapeInput, 0, 0, None, ShapeSet);
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

void NativeWindowViews::SetMenu(ui::MenuModel* menu_model) {
  if (menu_model == nullptr) {
    // Remove accelerators
    accelerator_table_.clear();
    GetFocusManager()->UnregisterAccelerators(this);
    // and menu bar.
#if defined(USE_X11)
    global_menu_bar_.reset();
#endif
    SetMenuBarVisibility(false);
    menu_bar_.reset();
    return;
  }

  RegisterAccelerators(menu_model);

#if defined(USE_X11)
  if (!global_menu_bar_ && ShouldUseGlobalMenuBar())
    global_menu_bar_.reset(new GlobalMenuBarX11(this));

  // Use global application menu bar when possible.
  if (global_menu_bar_ && global_menu_bar_->IsServerStarted()) {
    global_menu_bar_->SetMenu(menu_model);
    return;
  }
#endif

  // Do not show menu bar in frameless window.
  if (!has_frame())
    return;

  if (!menu_bar_) {
    gfx::Size content_size = GetContentSize();
    menu_bar_.reset(new MenuBar);
    menu_bar_->set_owned_by_client();

    if (!menu_bar_autohide_) {
      SetMenuBarVisibility(true);
      if (use_content_size_) {
        // Enlarge the size constraints for the menu.
        extensions::SizeConstraints constraints = GetContentSizeConstraints();
        if (constraints.HasMinimumSize()) {
          gfx::Size min_size = constraints.GetMinimumSize();
          min_size.set_height(min_size.height() + kMenuBarHeight);
          constraints.set_minimum_size(min_size);
        }
        if (constraints.HasMaximumSize()) {
          gfx::Size max_size = constraints.GetMaximumSize();
          max_size.set_height(max_size.height() + kMenuBarHeight);
          constraints.set_maximum_size(max_size);
        }
        SetContentSizeConstraints(constraints);

        // Resize the window to make sure content size is not changed.
        SetContentSize(content_size);
      }
    }
  }

  menu_bar_->SetMenu(menu_model);
  Layout();
}

void NativeWindowViews::SetParentWindow(NativeWindow* parent) {
  NativeWindow::SetParentWindow(parent);

#if defined(USE_X11)
  XDisplay* xdisplay = gfx::GetXDisplay();
  XSetTransientForHint(
      xdisplay, GetAcceleratedWidget(),
      parent? parent->GetAcceleratedWidget() : DefaultRootWindow(xdisplay));
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

gfx::NativeWindow NativeWindowViews::GetNativeWindow() {
  return window_->GetNativeWindow();
}

void NativeWindowViews::SetProgressBar(double progress) {
#if defined(OS_WIN)
  taskbar_host_.SetProgressBar(GetAcceleratedWidget(), progress);
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
  menu_bar_autohide_ = auto_hide;
}

bool NativeWindowViews::IsMenuBarAutoHide() {
  return menu_bar_autohide_;
}

void NativeWindowViews::SetMenuBarVisibility(bool visible) {
  if (!menu_bar_ || menu_bar_visible_ == visible)
    return;

  // Always show the accelerator when the auto-hide menu bar shows.
  if (menu_bar_autohide_)
    menu_bar_->SetAcceleratorVisibility(visible);

  menu_bar_visible_ = visible;
  if (visible) {
    DCHECK_EQ(child_count(), 1);
    AddChildView(menu_bar_.get());
  } else {
    DCHECK_EQ(child_count(), 2);
    RemoveChildView(menu_bar_.get());
  }

  Layout();
}

bool NativeWindowViews::IsMenuBarVisible() {
  return menu_bar_visible_;
}

void NativeWindowViews::SetVisibleOnAllWorkspaces(bool visible) {
  window_->SetVisibleOnAllWorkspaces(visible);
}

bool NativeWindowViews::IsVisibleOnAllWorkspaces() {
#if defined(USE_X11)
  // Use the presence/absence of _NET_WM_STATE_STICKY in _NET_WM_STATE to
  // determine whether the current window is visible on all workspaces.
  XAtom sticky_atom = GetAtom("_NET_WM_STATE_STICKY");
  std::vector<XAtom> wm_states;
  ui::GetAtomArrayProperty(GetAcceleratedWidget(), "_NET_WM_STATE", &wm_states);
  return std::find(wm_states.begin(),
                   wm_states.end(), sticky_atom) != wm_states.end();
#endif
  return false;
}

gfx::AcceleratedWidget NativeWindowViews::GetAcceleratedWidget() {
  return GetNativeWindow()->GetHost()->GetAcceleratedWidget();
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
  views::DesktopWindowTreeHostX11* tree_host =
      views::DesktopWindowTreeHostX11::GetHostForXID(GetAcceleratedWidget());
  static_cast<views::DesktopWindowTreeHost*>(tree_host)->SetWindowIcons(
      icon, icon);
}
#endif

void NativeWindowViews::SetEnabled(bool enable) {
  // Handle multiple calls of SetEnabled correctly.
  if (enable) {
    --disable_count_;
    if (disable_count_ != 0)
      return;
  } else {
    ++disable_count_;
    if (disable_count_ != 1)
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

void NativeWindowViews::OnWidgetActivationChanged(
    views::Widget* widget, bool active) {
  if (widget != window_.get())
    return;

  // Post the notification to next tick.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(active ? &NativeWindow::NotifyWindowFocus :
                          &NativeWindow::NotifyWindowBlur,
                 GetWeakPtr()));

  if (active && inspectable_web_contents() &&
      !inspectable_web_contents()->IsDevToolsViewShowing())
    web_contents()->Focus();

  // Hide menu bar when window is blured.
  if (!active && menu_bar_autohide_ && menu_bar_visible_)
    SetMenuBarVisibility(false);
}

void NativeWindowViews::OnWidgetBoundsChanged(
    views::Widget* widget, const gfx::Rect& bounds) {
  if (widget != window_.get())
    return;

  if (widget_size_ != bounds.size()) {
    NotifyWindowResize();
    widget_size_ = bounds.size();
  }
}

void NativeWindowViews::DeleteDelegate() {
  if (is_modal() && NativeWindow::parent()) {
    NativeWindowViews* parent =
        static_cast<NativeWindowViews*>(NativeWindow::parent());
    // Enable parent window after current window gets closed.
    parent->SetEnabled(true);
    // Focus on parent window.
    parent->Focus(true);
  }

  NotifyWindowClosed();
}

views::View* NativeWindowViews::GetInitiallyFocusedView() {
  return inspectable_web_contents()->GetView()->GetWebView();
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

bool NativeWindowViews::ShouldHandleSystemCommands() const {
  return true;
}

views::Widget* NativeWindowViews::GetWidget() {
  return window_.get();
}

const views::Widget* NativeWindowViews::GetWidget() const {
  return window_.get();
}

views::View* NativeWindowViews::GetContentsView() {
  return this;
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
    FramelessView* frame = static_cast<FramelessView*>(
        window_->non_client_view()->frame_view());
    return frame->ResizingBorderHitTest(location) == HTNOWHERE;
  }

  return true;
}

views::ClientView* NativeWindowViews::CreateClientView(views::Widget* widget) {
  return new NativeWindowClientView(widget, this);
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

gfx::Size NativeWindowViews::ContentSizeToWindowSize(const gfx::Size& size) {
  if (!has_frame())
    return size;

  gfx::Size window_size(size);
#if defined(OS_WIN)
  gfx::Rect dpi_bounds =
      gfx::Rect(gfx::Point(), gfx::win::DIPToScreenSize(size));
  gfx::Rect window_bounds = gfx::win::ScreenToDIPRect(
      window_->non_client_view()->GetWindowBoundsForClientBounds(dpi_bounds));
  window_size = window_bounds.size();
#endif

  if (menu_bar_ && menu_bar_visible_)
    window_size.set_height(window_size.height() + kMenuBarHeight);
  return window_size;
}

gfx::Size NativeWindowViews::WindowSizeToContentSize(const gfx::Size& size) {
  if (!has_frame())
    return size;

  gfx::Size content_size(size);
#if defined(OS_WIN)
  content_size = gfx::win::DIPToScreenSize(content_size);
  RECT rect;
  SetRectEmpty(&rect);
  HWND hwnd = GetAcceleratedWidget();
  DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
  DWORD ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
  AdjustWindowRectEx(&rect, style, FALSE, ex_style);
  content_size.set_width(content_size.width() - (rect.right - rect.left));
  content_size.set_height(content_size.height() - (rect.bottom - rect.top));
  content_size = gfx::win::ScreenToDIPSize(content_size);
#endif

  if (menu_bar_ && menu_bar_visible_)
    content_size.set_height(content_size.height() - kMenuBarHeight);
  return content_size;
}

void NativeWindowViews::HandleKeyboardEvent(
    content::WebContents*,
    const content::NativeWebKeyboardEvent& event) {
  keyboard_event_handler_->HandleKeyboardEvent(event, GetFocusManager());

  if (!menu_bar_)
    return;

  // Show accelerator when "Alt" is pressed.
  if (menu_bar_visible_ && IsAltKey(event))
    menu_bar_->SetAcceleratorVisibility(
        event.type == blink::WebInputEvent::RawKeyDown);

  // Show the submenu when "Alt+Key" is pressed.
  if (event.type == blink::WebInputEvent::RawKeyDown && !IsAltKey(event) &&
      IsAltModifier(event)) {
    if (!menu_bar_visible_ &&
        (menu_bar_->GetAcceleratorIndex(event.windowsKeyCode) != -1))
      SetMenuBarVisibility(true);
    menu_bar_->ActivateAccelerator(event.windowsKeyCode);
    return;
  }

  if (!menu_bar_autohide_)
    return;

  // Toggle the menu bar only when a single Alt is released.
  if (event.type == blink::WebInputEvent::RawKeyDown && IsAltKey(event)) {
    // When a single Alt is pressed:
    menu_bar_alt_pressed_ = true;
  } else if (event.type == blink::WebInputEvent::KeyUp && IsAltKey(event) &&
             menu_bar_alt_pressed_) {
    // When a single Alt is released right after a Alt is pressed:
    menu_bar_alt_pressed_ = false;
    SetMenuBarVisibility(!menu_bar_visible_);
  } else {
    // When any other keys except single Alt have been pressed/released:
    menu_bar_alt_pressed_ = false;
  }
}

gfx::Size NativeWindowViews::GetMinimumSize() {
  return NativeWindow::GetMinimumSize();
}

gfx::Size NativeWindowViews::GetMaximumSize() {
  return NativeWindow::GetMaximumSize();
}

bool NativeWindowViews::AcceleratorPressed(const ui::Accelerator& accelerator) {
  return accelerator_util::TriggerAcceleratorTableCommand(
      &accelerator_table_, accelerator);
}

void NativeWindowViews::RegisterAccelerators(ui::MenuModel* menu_model) {
  // Clear previous accelerators.
  views::FocusManager* focus_manager = GetFocusManager();
  accelerator_table_.clear();
  focus_manager->UnregisterAccelerators(this);

  // Register accelerators with focus manager.
  accelerator_util::GenerateAcceleratorTable(&accelerator_table_, menu_model);
  accelerator_util::AcceleratorTable::const_iterator iter;
  for (iter = accelerator_table_.begin();
       iter != accelerator_table_.end();
       ++iter) {
    focus_manager->RegisterAccelerator(
        iter->first, ui::AcceleratorManager::kNormalPriority, this);
  }
}

ui::WindowShowState NativeWindowViews::GetRestoredState() {
  if (IsMaximized())
    return ui::SHOW_STATE_MAXIMIZED;
  if (IsFullscreen())
    return ui::SHOW_STATE_FULLSCREEN;

  return ui::SHOW_STATE_NORMAL;
}

// static
NativeWindow* NativeWindow::Create(
    brightray::InspectableWebContents* inspectable_web_contents,
    const mate::Dictionary& options,
    NativeWindow* parent) {
  return new NativeWindowViews(inspectable_web_contents, options, parent);
}

}  // namespace atom
