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
#include "shell/browser/native_window_features.h"
#include "shell/browser/ui/views/client_frame_view_linux.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/platform_window/platform_window.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_linux.h"
#include "ui/views/window/non_client_view.h"

#if defined(USE_OZONE)
#include "ui/ozone/buildflags.h"
#if BUILDFLAG(OZONE_PLATFORM_X11)
#define USE_OZONE_PLATFORM_X11
#endif
#include "ui/ozone/public/ozone_platform.h"
#endif

namespace electron {

ElectronDesktopWindowTreeHostLinux::ElectronDesktopWindowTreeHostLinux(
    NativeWindowViews* native_window_view,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
    : views::DesktopWindowTreeHostLinux(native_window_view->widget(),
                                        desktop_native_widget_aura),
      native_window_view_(native_window_view) {}

ElectronDesktopWindowTreeHostLinux::~ElectronDesktopWindowTreeHostLinux() =
    default;

bool ElectronDesktopWindowTreeHostLinux::SupportsClientFrameShadow() const {
  return platform_window()->CanSetDecorationInsets() &&
         platform_window()->IsTranslucentWindowOpacitySupported();
}

void ElectronDesktopWindowTreeHostLinux::OnWidgetInitDone() {
  views::DesktopWindowTreeHostLinux::OnWidgetInitDone();
  UpdateFrameHints();
}

void ElectronDesktopWindowTreeHostLinux::OnBoundsChanged(
    const BoundsChange& change) {
  views::DesktopWindowTreeHostLinux::OnBoundsChanged(change);
  UpdateFrameHints();

#if defined(USE_OZONE_PLATFORM_X11)
  if (ui::OzonePlatform::GetInstance()
          ->GetPlatformProperties()
          .electron_can_call_x11) {
    // The OnWindowStateChanged should receive all updates but currently under
    // X11 it doesn't receive changes to the fullscreen status because chromium
    // is handling the fullscreen state changes synchronously, see
    // X11Window::ToggleFullscreen in ui/ozone/platform/x11/x11_window.cc.
    UpdateWindowState(platform_window()->GetPlatformWindowState());
  }
#endif
}

void ElectronDesktopWindowTreeHostLinux::OnWindowStateChanged(
    ui::PlatformWindowState old_state,
    ui::PlatformWindowState new_state) {
  views::DesktopWindowTreeHostLinux::OnWindowStateChanged(old_state, new_state);
  UpdateFrameHints();
  UpdateWindowState(new_state);
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
    if (SupportsClientFrameShadow() && native_window_view_->has_frame() &&
        native_window_view_->has_client_frame()) {
      UpdateClientDecorationHints(static_cast<ClientFrameViewLinux*>(
          native_window_view_->widget()->non_client_view()->frame_view()));
    }

    SizeConstraintsChanged();
  }
}

void ElectronDesktopWindowTreeHostLinux::UpdateClientDecorationHints(
    ClientFrameViewLinux* view) {
  ui::PlatformWindow* window = platform_window();
  bool showing_frame = !native_window_view_->IsFullscreen();
  float scale = device_scale_factor();

  bool should_set_opaque_region = window->IsTranslucentWindowOpacitySupported();

  gfx::Insets insets;
  gfx::Insets input_insets;
  if (showing_frame) {
    insets = view->GetBorderDecorationInsets();
    if (base::i18n::IsRTL()) {
      insets.Set(insets.top(), insets.right(), insets.bottom(), insets.left());
    }

    input_insets = view->GetInputInsets();
  }

  gfx::Insets scaled_insets = gfx::ScaleToCeiledInsets(insets, scale);
  window->SetDecorationInsets(&scaled_insets);

  gfx::Rect input_bounds(view->GetWidget()->GetWindowBoundsInScreen().size());
  input_bounds.Inset(insets + input_insets);
  gfx::Rect scaled_bounds = gfx::ScaleToEnclosingRect(input_bounds, scale);
  window->SetInputRegion(&scaled_bounds);

  if (should_set_opaque_region) {
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

    // Convert the region to a list of rectangles.
    std::vector<gfx::Rect> opaque_region;
    for (SkRegion::Iterator i(region); !i.done(); i.next())
      opaque_region.push_back(gfx::SkIRectToRect(i.rect()));
    window->SetOpaqueRegion(&opaque_region);
  }
}

}  // namespace electron
