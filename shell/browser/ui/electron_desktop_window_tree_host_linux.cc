// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
// Portions of this file are sourced from
// chrome/browser/ui/views/frame/browser_desktop_window_tree_host_linux.cc,
// Copyright (c) 2019 The Chromium Authors,
// which is governed by a BSD-style license

#include "shell/browser/ui/electron_desktop_window_tree_host_linux.h"

#include <vector>

#include "base/i18n/rtl.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/linux/x11_util.h"
#include "shell/browser/native_window_views.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/hit_test.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/linux/linux_ui.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/platform_window/extensions/wayland_extension.h"
#include "ui/platform_window/platform_window.h"
#include "ui/platform_window/platform_window_init_properties.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_linux.h"
#include "ui/views/window/frame_view_linux.h"
#include "ui/views/window/frame_view_utils_linux.h"

namespace electron {

ElectronDesktopWindowTreeHostLinux::ElectronDesktopWindowTreeHostLinux(
    NativeWindowViews* native_window_view,
    views::Widget* widget,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
    : views::DesktopWindowTreeHostLinux{widget, desktop_native_widget_aura},
      native_window_view_{native_window_view} {}

ElectronDesktopWindowTreeHostLinux::~ElectronDesktopWindowTreeHostLinux() =
    default;

bool ElectronDesktopWindowTreeHostLinux::SupportsClientFrameShadow() const {
  return platform_window()->CanSetDecorationInsets() &&
         views::Widget::IsWindowCompositingSupported();
}

void ElectronDesktopWindowTreeHostLinux::OnWidgetInitDone() {
  views::DesktopWindowTreeHostLinux::OnWidgetInitDone();

  // SetSupportsClientFrameShadow must happen after widget init when
  // platform_window is available.
  if (auto* fvl = native_window_view_->GetFrameViewLinux())
    fvl->SetSupportsClientFrameShadow(SupportsClientFrameShadow() &&
                                      !native_window_view_->IsTranslucent());

  UpdateFrameHints();
}

bool ElectronDesktopWindowTreeHostLinux::IsShowingFrame(
    ui::PlatformWindowState window_state) const {
  return window_state != ui::PlatformWindowState::kFullScreen &&
         window_state != ui::PlatformWindowState::kMaximized &&
         window_state != ui::PlatformWindowState::kMinimized;
}

void ElectronDesktopWindowTreeHostLinux::SetWindowIcons(
    const gfx::ImageSkia& window_icon,
    const gfx::ImageSkia& app_icon) {
  DesktopWindowTreeHostLinux::SetWindowIcons(window_icon, app_icon);

  if (ui::GetWaylandToplevelExtension(*platform_window()))
    saved_window_icon_ = window_icon;
}

void ElectronDesktopWindowTreeHostLinux::Show(
    ui::mojom::WindowShowState show_state,
    const gfx::Rect& restore_bounds) {
  DesktopWindowTreeHostLinux::Show(show_state, restore_bounds);

  if (!saved_window_icon_.isNull())
    DesktopWindowTreeHostLinux::SetWindowIcons(saved_window_icon_, {});
}

gfx::Insets ElectronDesktopWindowTreeHostLinux::GetRestoredFrameBorderInsets()
    const {
  if (auto* fvl = native_window_view_->GetFrameViewLinux())
    return fvl->GetRestoredFrameBorderInsets();

  return gfx::Insets();
}

gfx::Insets ElectronDesktopWindowTreeHostLinux::CalculateInsetsInDIP(
    ui::PlatformWindowState window_state) const {
  // If we are not showing frame, the insets should be zero.
  if (!IsShowingFrame(window_state))
    return gfx::Insets();

  return GetRestoredFrameBorderInsets();
}

// Electron treats min/max constraints as the logical window size, but Chromium
// expects widget bounds including CSD insets (WaylandToplevelWindow::
// SetSizeConstraints). So we inflate constraints by insets to avoid double
// subtraction. This is still OK for SSD frames or X11 where the insets are 0.
std::optional<gfx::Size>
ElectronDesktopWindowTreeHostLinux::GetMinimumSizeForWindow() const {
  auto min_size = views::DesktopWindowTreeHostLinux::GetMinimumSizeForWindow();
  if (min_size.has_value()) {
    gfx::Insets insets = GetRestoredFrameBorderInsets();
    min_size->Enlarge(insets.width(), insets.height());
  }
  return min_size;
}

std::optional<gfx::Size>
ElectronDesktopWindowTreeHostLinux::GetMaximumSizeForWindow() const {
  auto max_size = views::DesktopWindowTreeHostLinux::GetMaximumSizeForWindow();
  if (max_size.has_value()) {
    gfx::Insets insets = GetRestoredFrameBorderInsets();
    // 0 means no constraint, so don't inflate.
    if (max_size->width() > 0)
      max_size->set_width(max_size->width() + insets.width());
    if (max_size->height() > 0)
      max_size->set_height(max_size->height() + insets.height());
  }
  return max_size;
}

void ElectronDesktopWindowTreeHostLinux::OnBoundsChanged(
    const BoundsChange& change) {
  views::DesktopWindowTreeHostLinux::OnBoundsChanged(change);
  UpdateFrameHints();

  if (x11_util::IsX11()) {
    // The OnWindowStateChanged should receive all updates but currently under
    // X11 it doesn't receive changes to the fullscreen status because chromium
    // is handling the fullscreen state changes synchronously, see
    // X11Window::ToggleFullscreen in ui/ozone/platform/x11/x11_window.cc.
    UpdateWindowState(platform_window()->GetPlatformWindowState());
  }
}

void ElectronDesktopWindowTreeHostLinux::OnWindowStateChanged(
    ui::PlatformWindowState old_state,
    ui::PlatformWindowState new_state) {
  views::DesktopWindowTreeHostLinux::OnWindowStateChanged(old_state, new_state);
  UpdateFrameHints();
  UpdateWindowState(new_state);
}

void ElectronDesktopWindowTreeHostLinux::OnWindowTiledStateChanged(
    ui::WindowTiledEdges new_tiled_edges) {
  // GNOME on Ubuntu reports all edges as tiled even if the window is only
  // half-tiled, so do not trust individual edge values.
  bool maximized = native_window_view_->IsMaximized();
  bool tiled = new_tiled_edges.top || new_tiled_edges.left ||
               new_tiled_edges.bottom || new_tiled_edges.right;
  bool is_tiled = tiled && !maximized;

  if (auto* fvl = native_window_view_->GetFrameViewLinux())
    fvl->SetTiled(is_tiled);
  UpdateFrameHints();
  ScheduleRelayout();
  if (GetWidget()->non_client_view()) {
    GetWidget()->non_client_view()->SchedulePaint();
  }
}

void ElectronDesktopWindowTreeHostLinux::UpdateWindowState(
    ui::PlatformWindowState new_state) {
  if (window_state_ == new_state)
    return;

  switch (window_state_) {
    case ui::PlatformWindowState::kMinimized:
      native_window_view_->NotifyWindowRestore();
      break;
    case ui::PlatformWindowState::kMaximized:
      native_window_view_->NotifyWindowUnmaximize();
      break;
    case ui::PlatformWindowState::kFullScreen:
      native_window_view_->NotifyWindowLeaveFullScreen();
      break;
    case ui::PlatformWindowState::kUnknown:
    case ui::PlatformWindowState::kNormal:
      break;
  }
  switch (new_state) {
    case ui::PlatformWindowState::kMinimized:
      native_window_view_->NotifyWindowMinimize();
      break;
    case ui::PlatformWindowState::kMaximized:
      native_window_view_->NotifyWindowMaximize();
      break;
    case ui::PlatformWindowState::kFullScreen:
      native_window_view_->NotifyWindowEnterFullScreen();
      break;
    case ui::PlatformWindowState::kUnknown:
    case ui::PlatformWindowState::kNormal:
      break;
  }
  window_state_ = new_state;
}

void ElectronDesktopWindowTreeHostLinux::OnNativeThemeUpdated(
    ui::NativeTheme* observed_theme) {
  UpdateFrameHints();
}

void ElectronDesktopWindowTreeHostLinux::OnDeviceScaleFactorChanged() {
  UpdateFrameHints();
}

void ElectronDesktopWindowTreeHostLinux::SetOpacity(float opacity) {
  views::DesktopWindowTreeHostLinux::SetOpacity(opacity);
  UpdateFrameHints();
}

void ElectronDesktopWindowTreeHostLinux::UpdateFrameHints() {
  const bool is_non_opaque = native_window_view_->IsTranslucent() ||
                             native_window_view_->GetOpacity() < 1.0;

  auto* fvl = native_window_view_->GetFrameViewLinux();
  if (!fvl || !fvl->ShouldDrawRestoredFrameShadow()) {
    platform_window()->SetInputRegion(std::nullopt);
    if (ui::OzonePlatform::GetInstance()->IsWindowCompositingSupported()) {
      if (is_non_opaque) {
        platform_window()->SetOpaqueRegion(std::vector<gfx::Rect>{});
      } else {
        gfx::Size size = GetWidget()->GetWindowBoundsInScreen().size();
        float scale = device_scale_factor();
        platform_window()->SetOpaqueRegion(std::vector<gfx::Rect>{
            gfx::ScaleToEnclosingRect(gfx::Rect(size), scale)});
      }
    }
    SizeConstraintsChanged();
    return;
  }

  views::DesktopWindowTreeHostLinux::UpdateFrameHints();
  if (is_non_opaque && views::Widget::IsWindowCompositingSupported()) {
    platform_window()->SetOpaqueRegion(std::vector<gfx::Rect>{});
  }
  SizeConstraintsChanged();
}

void ElectronDesktopWindowTreeHostLinux::DispatchEvent(ui::Event* event) {
  if (event->IsMouseEvent()) {
    auto* mouse_event = static_cast<ui::MouseEvent*>(event);
    bool is_mousedown = mouse_event->type() == ui::EventType::kMousePressed;
    bool is_system_menu_trigger =
        is_mousedown &&
        (mouse_event->IsRightMouseButton() ||
         (mouse_event->IsLeftMouseButton() && mouse_event->IsControlDown()));

    if (!is_system_menu_trigger) {
      views::DesktopWindowTreeHostLinux::DispatchEvent(event);
      return;
    }

    // Determine the non-client area and dispatch 'system-context-menu'.
    if (GetContentWindow() && GetContentWindow()->delegate()) {
      ui::LocatedEvent* located_event = event->AsLocatedEvent();
      gfx::PointF location = located_event->location_f();
      gfx::PointF location_in_dip =
          GetRootTransform().InverseMapPoint(location).value_or(location);
      int hit_test_code = GetContentWindow()->delegate()->GetNonClientComponent(
          gfx::ToRoundedPoint(location_in_dip));
      if (hit_test_code != HTCLIENT && hit_test_code != HTNOWHERE) {
        bool prevent_default = false;
        native_window_view_->NotifyWindowSystemContextMenu(
            located_event->x(), located_event->y(), &prevent_default);

        // If |prevent_default| is true, then the user might want to show a
        // custom menu - proceed propagation and emit context-menu in the
        // renderer. Otherwise, show the native system window controls menu.
        if (prevent_default) {
          electron::api::WebContents::SetDisableDraggableRegions(true);
          views::DesktopWindowTreeHostLinux::DispatchEvent(event);
          electron::api::WebContents::SetDisableDraggableRegions(false);
        } else {
          if (ui::OzonePlatform::GetInstance()
                  ->GetPlatformRuntimeProperties()
                  .supports_server_window_menus) {
            views::DesktopWindowTreeHostLinux::ShowWindowControlsMenu(
                display::Screen::Get()->GetCursorScreenPoint());
          }
        }
        return;
      }
    }
  }

  views::DesktopWindowTreeHostLinux::DispatchEvent(event);
}

void ElectronDesktopWindowTreeHostLinux::AddAdditionalInitProperties(
    const views::Widget::InitParams& params,
    ui::PlatformWindowInitProperties* properties) {
  views::DesktopWindowTreeHostLinux::AddAdditionalInitProperties(params,
                                                                 properties);
  const auto* linux_ui_theme = ui::LinuxUiTheme::GetForProfile(nullptr);
  properties->prefer_dark_theme =
      linux_ui_theme && linux_ui_theme->PreferDarkTheme();
}

}  // namespace electron
