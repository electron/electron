// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/atom_desktop_window_tree_host_win.h"

#include "ui/aura/client/aura_constants.h"
#include "ui/base/win/shell.h"
#include "ui/display/win/screen_win.h"
#include "ui/display/win/screen_win_display.h"
#include "ui/views/widget/widget_hwnd_utils.h"
#include "ui/views/win/hwnd_message_handler.h"

using display::win::ScreenWin;
using display::win::ScreenWinDisplay;

namespace electron {

namespace {

gfx::Size GetExpandedWindowSize(bool is_translucent, gfx::Size size) {
  if (!is_translucent || !ui::win::IsAeroGlassEnabled())
    return size;

  // Some AMD drivers can't display windows that are less than 64x64 pixels,
  // so expand them to be at least that size. http://crbug.com/286609
  gfx::Size expanded(std::max(size.width(), 64), std::max(size.height(), 64));
  return expanded;
}

void InsetBottomRight(gfx::Rect* rect, const gfx::Vector2d& vector) {
  rect->Inset(0, 0, vector.x(), vector.y());
}

gfx::Rect DIPToScreenRect(HWND hwnd, const gfx::Rect& dip_bounds) {
  const ScreenWinDisplay screen_win_display =
      hwnd ? display::win::ScreenWin::GetScreenWinDisplayVia(
                 &ScreenWin::GetScreenWinDisplayNearestHWND, hwnd)
           : display::win::ScreenWin::GetScreenWinDisplayVia(
                 &ScreenWin::GetScreenWinDisplayNearestDIPRect, dip_bounds);
  float scale_factor = screen_win_display.display().device_scale_factor();
  return ScaleToRoundedRect(dip_bounds, scale_factor);
}

gfx::Rect ScreenToDIPRect(HWND hwnd, const gfx::Rect& pixel_bounds) {
  const ScreenWinDisplay screen_win_display =
      hwnd
          ? display::win::ScreenWin::GetScreenWinDisplayVia(
                &ScreenWin::GetScreenWinDisplayNearestHWND, hwnd)
          : display::win::ScreenWin::GetScreenWinDisplayVia(
                &ScreenWin::GetScreenWinDisplayNearestScreenRect, pixel_bounds);
  float scale_factor = screen_win_display.display().device_scale_factor();
  return ScaleToRoundedRect(pixel_bounds, 1.0f / scale_factor);
}

gfx::Size DIPToScreenSize(HWND hwnd, const gfx::Size& dip_size) {
  float scale_factor = display::win::ScreenWin::GetScaleFactorForHWND(hwnd);
  return ScaleToRoundedSize(dip_size, scale_factor);
}

gfx::Rect DIPToScreenRectInWindow(gfx::NativeView view,
                                  const gfx::Rect& dip_rect) {
  auto* screen = display::Screen::GetScreen();
  float scale = screen->GetDisplayNearestView(view).device_scale_factor();
  return ScaleToRoundedRect(dip_rect, scale);
}

}  // namespace

AtomDesktopWindowTreeHostWin::AtomDesktopWindowTreeHostWin(
    NativeWindowViews* native_window_view,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
    : views::DesktopWindowTreeHostWin(native_window_view->widget(),
                                      desktop_native_widget_aura),
      native_window_view_(native_window_view) {}

AtomDesktopWindowTreeHostWin::~AtomDesktopWindowTreeHostWin() {}

void AtomDesktopWindowTreeHostWin::Init(
    const views::Widget::InitParams& params) {
  wants_mouse_events_when_inactive_ = params.wants_mouse_events_when_inactive;

  wm::SetAnimationHost(content_window(), this);
  if (params.type == views::Widget::InitParams::TYPE_WINDOW &&
      !params.remove_standard_frame)
    content_window()->SetProperty(aura::client::kAnimationsDisabledKey, true);

  ConfigureWindowStyles(message_handler_.get(), params,
                        GetWidget()->widget_delegate(),
                        native_widget_delegate_);

  HWND parent_hwnd = nullptr;
  if (params.parent && params.parent->GetHost())
    parent_hwnd = params.parent->GetHost()->GetAcceleratedWidget();

  remove_standard_frame_ = params.remove_standard_frame;
  has_non_client_view_ = views::Widget::RequiresNonClientView(params.type);
  z_order_ = params.EffectiveZOrderLevel();

  // We don't have an HWND yet, so scale relative to the nearest screen.
  gfx::Rect pixel_bounds = DIPToScreenRect(nullptr, params.bounds);
  message_handler_->Init(parent_hwnd, pixel_bounds);
  CreateCompositor(viz::FrameSinkId(), params.force_software_compositing);
  OnAcceleratedWidgetAvailable();
  InitHost();
  window()->Show();
}

void AtomDesktopWindowTreeHostWin::Show(ui::WindowShowState show_state,
                                        const gfx::Rect& restore_bounds) {
  if (compositor())
    compositor()->SetVisible(true);

  gfx::Rect pixel_restore_bounds;
  if (show_state == ui::SHOW_STATE_MAXIMIZED) {
    pixel_restore_bounds = DIPToScreenRect(GetHWND(), restore_bounds);
  }
  message_handler_->Show(show_state, pixel_restore_bounds);

  content_window()->Show();
}

void AtomDesktopWindowTreeHostWin::SetSize(const gfx::Size& size) {
  gfx::Size size_in_pixels = electron::DIPToScreenSize(GetHWND(), size);
  gfx::Size expanded =
      GetExpandedWindowSize(message_handler_->is_translucent(), size_in_pixels);
  window_enlargement_ =
      gfx::Vector2d(expanded.width() - size_in_pixels.width(),
                    expanded.height() - size_in_pixels.height());
  message_handler_->SetSize(expanded);
}

void AtomDesktopWindowTreeHostWin::SetBoundsInDIP(const gfx::Rect& bounds) {
  aura::Window* root = AsWindowTreeHost()->window();
  const gfx::Rect bounds_in_pixels =
      electron::DIPToScreenRectInWindow(root, bounds);
  AsWindowTreeHost()->SetBoundsInPixels(bounds_in_pixels);
}

void AtomDesktopWindowTreeHostWin::CenterWindow(const gfx::Size& size) {
  gfx::Size size_in_pixels = electron::DIPToScreenSize(GetHWND(), size);
  gfx::Size expanded_size;
  expanded_size =
      GetExpandedWindowSize(message_handler_->is_translucent(), size_in_pixels);
  window_enlargement_ =
      gfx::Vector2d(expanded_size.width() - size_in_pixels.width(),
                    expanded_size.height() - size_in_pixels.height());
  message_handler_->CenterWindow(expanded_size);
}

void AtomDesktopWindowTreeHostWin::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  message_handler_->GetWindowPlacement(bounds, show_state);
  InsetBottomRight(bounds, window_enlargement_);
  *bounds = electron::ScreenToDIPRect(GetHWND(), *bounds);
}

gfx::Rect AtomDesktopWindowTreeHostWin::GetWindowBoundsInScreen() const {
  gfx::Rect pixel_bounds = message_handler_->GetWindowBoundsInScreen();
  InsetBottomRight(&pixel_bounds, window_enlargement_);
  return electron::ScreenToDIPRect(GetHWND(), pixel_bounds);
}

gfx::Rect AtomDesktopWindowTreeHostWin::GetClientAreaBoundsInScreen() const {
  gfx::Rect pixel_bounds = message_handler_->GetClientAreaBoundsInScreen();
  InsetBottomRight(&pixel_bounds, window_enlargement_);
  return electron::ScreenToDIPRect(GetHWND(), pixel_bounds);
}

gfx::Rect AtomDesktopWindowTreeHostWin::GetRestoredBounds() const {
  gfx::Rect pixel_bounds = message_handler_->GetRestoredBounds();
  InsetBottomRight(&pixel_bounds, window_enlargement_);
  return electron::ScreenToDIPRect(GetHWND(), pixel_bounds);
}

gfx::Rect AtomDesktopWindowTreeHostWin::GetWorkAreaBoundsInScreen() const {
  MONITORINFO monitor_info;
  monitor_info.cbSize = sizeof(monitor_info);
  GetMonitorInfo(
      MonitorFromWindow(message_handler_->hwnd(), MONITOR_DEFAULTTONEAREST),
      &monitor_info);
  gfx::Rect pixel_bounds = gfx::Rect(monitor_info.rcWork);
  return electron::ScreenToDIPRect(GetHWND(), pixel_bounds);
}

bool AtomDesktopWindowTreeHostWin::PreHandleMSG(UINT message,
                                                WPARAM w_param,
                                                LPARAM l_param,
                                                LRESULT* result) {
  return native_window_view_->PreHandleMSG(message, w_param, l_param, result);
}

bool AtomDesktopWindowTreeHostWin::HasNativeFrame() const {
  // Since we never use chromium's titlebar implementation, we can just say
  // that we use a native titlebar. This will disable the repaint locking when
  // DWM composition is disabled.
  return true;
}

}  // namespace electron
