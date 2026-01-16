// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
// Portions of this file are sourced from
// chrome/browser/ui/views/frame/browser_desktop_window_tree_host_linux.cc,
// Copyright (c) 2019 The Chromium Authors,
// which is governed by a BSD-style license

#include "shell/browser/ui/electron_desktop_window_tree_host_linux.h"

#include <vector>

#include "base/feature_list.h"
#include "base/i18n/rtl.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/linux/x11_util.h"
#include "shell/browser/native_window_features.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/client_frame_view_linux.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/hit_test.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/linux/linux_ui.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/platform_window/extensions/wayland_extension.h"
#include "ui/platform_window/platform_window.h"
#include "ui/platform_window/platform_window_init_properties.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_linux.h"

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
  UpdateFrameHints();
}

bool ElectronDesktopWindowTreeHostLinux::IsShowingFrame() const {
  return !native_window_view_->IsFullscreen() &&
         !native_window_view_->IsMaximized() &&
         !native_window_view_->IsMinimized();
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

gfx::Insets ElectronDesktopWindowTreeHostLinux::CalculateInsetsInDIP(
    ui::PlatformWindowState window_state) const {
  // If we are not showing frame, the insets should be zero.
  if (!IsShowingFrame()) {
    return gfx::Insets();
  }

  auto* const view = native_window_view_->GetClientFrameViewLinux();
  if (!view)
    return {};

  gfx::Insets insets = view->RestoredMirroredFrameBorderInsets();
  if (base::i18n::IsRTL())
    insets.set_left_right(insets.right(), insets.left());
  return insets;
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
  if (auto* const view = native_window_view_->GetClientFrameViewLinux()) {
    // GNOME on Ubuntu reports all edges as tiled
    // even if the window is only half-tiled so do not trust individual edge
    // values.
    bool maximized = native_window_view_->IsMaximized();
    bool tiled = new_tiled_edges.top || new_tiled_edges.left ||
                 new_tiled_edges.bottom || new_tiled_edges.right;
    view->set_tiled(tiled && !maximized);
  }
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

void ElectronDesktopWindowTreeHostLinux::UpdateFrameHints() {
  if (base::FeatureList::IsEnabled(features::kWaylandWindowDecorations)) {
    auto* const view = native_window_view_->GetClientFrameViewLinux();
    if (!view)
      return;

    ui::PlatformWindow* window = platform_window();
    auto window_state = window->GetPlatformWindowState();
    float scale = device_scale_factor();
    const gfx::Size widget_size =
        view->GetWidget()->GetWindowBoundsInScreen().size();

    if (SupportsClientFrameShadow()) {
      auto insets = CalculateInsetsInDIP(window_state);
      if (insets.IsEmpty()) {
        window->SetInputRegion(std::nullopt);
      } else {
        gfx::Rect input_bounds(widget_size);
        input_bounds.Inset(insets - view->GetInputInsets());
        input_bounds = gfx::ScaleToEnclosingRect(input_bounds, scale);
        window->SetInputRegion(
            std::optional<std::vector<gfx::Rect>>({input_bounds}));
      }
    }

    if (ui::OzonePlatform::GetInstance()->IsWindowCompositingSupported()) {
      // Set the opaque region.
      std::vector<gfx::Rect> opaque_region;
      if (IsShowingFrame()) {
        // The opaque region is a list of rectangles that contain only fully
        // opaque pixels of the window.  We need to convert the clipping
        // rounded-rect into this format.
        SkRRect rrect = view->GetRoundedWindowContentBounds();
        gfx::RectF rectf(view->GetWindowContentBounds());
        rectf.Scale(scale);
        // It is acceptable to omit some pixels that are opaque, but the region
        // must not include any translucent pixels.  Therefore, we must
        // conservatively scale to the enclosed rectangle.
        gfx::Rect rect = gfx::ToEnclosedRect(rectf);

        // Create the initial region from the clipping rectangle without rounded
        // corners.
        SkRegion region(gfx::RectToSkIRect(rect));

        // Now subtract out the small rectangles that cover the corners.
        struct {
          SkRRect::Corner corner;
          bool left;
          bool upper;
        } kCorners[] = {
            {SkRRect::kUpperLeft_Corner, true, true},
            {SkRRect::kUpperRight_Corner, false, true},
            {SkRRect::kLowerLeft_Corner, true, false},
            {SkRRect::kLowerRight_Corner, false, false},
        };
        for (const auto& corner : kCorners) {
          auto radii = rrect.radii(corner.corner);
          auto rx = std::ceil(scale * radii.x());
          auto ry = std::ceil(scale * radii.y());
          auto corner_rect = SkIRect::MakeXYWH(
              corner.left ? rect.x() : rect.right() - rx,
              corner.upper ? rect.y() : rect.bottom() - ry, rx, ry);
          region.op(corner_rect, SkRegion::kDifference_Op);
        }

        auto translucent_top_area_rect = SkIRect::MakeXYWH(
            rect.x(), rect.y(), rect.width(),
            std::ceil(view->GetTranslucentTopAreaHeight() * scale - rect.y()));
        region.op(translucent_top_area_rect, SkRegion::kDifference_Op);

        // Convert the region to a list of rectangles.
        for (SkRegion::Iterator i(region); !i.done(); i.next()) {
          opaque_region.push_back(gfx::SkIRectToRect(i.rect()));
        }
      } else {
        // The entire window except for the translucent top is opaque.
        gfx::Rect opaque_region_dip(widget_size);
        gfx::Insets insets;
        insets.set_top(view->GetTranslucentTopAreaHeight());
        opaque_region_dip.Inset(insets);
        opaque_region.push_back(
            gfx::ScaleToEnclosingRect(opaque_region_dip, scale));
      }
      window->SetOpaqueRegion(opaque_region);
    }

    SizeConstraintsChanged();
  }
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
