// Copyright (c) 2025 Mitchell Cohen.
// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/linux_frame_layout.h"

#include <algorithm>

#include "base/i18n/rtl.h"
#include "chrome/browser/ui/views/frame/browser_frame_view_paint_utils_linux.h"  // nogncheck
#include "shell/browser/linux/x11_util.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/electron_desktop_window_tree_host_linux.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/linux/linux_ui.h"
#include "ui/linux/window_frame_provider.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"

namespace electron {

namespace {
// This should match Chromium's value.
constexpr int kResizeBorder = 10;
// This should match FramelessView's inside resize band.
constexpr int kResizeInsideBoundsSize = 5;
// These should match Chromium's restored frame edge thickness.
constexpr gfx::Insets kDefaultCustomFrameBorder = gfx::Insets::TLBR(2, 1, 1, 1);

bool CheckClientFrameShadowSupport(NativeWindowViews* window) {
  auto* tree_host = static_cast<ElectronDesktopWindowTreeHostLinux*>(
      ElectronDesktopWindowTreeHostLinux::GetHostForWidget(
          window->GetAcceleratedWidget()));
  return tree_host && tree_host->SupportsClientFrameShadow();
}
}  // namespace

// static
std::unique_ptr<LinuxFrameLayout> LinuxFrameLayout::Create(
    NativeWindowViews* window,
    bool wants_shadow,
    CSDStyle csd_style) {
  if (x11_util::IsX11() || window->IsTranslucent() || !wants_shadow) {
    return std::make_unique<LinuxFrameLayout>(window);
  } else if (csd_style == CSDStyle::kCustom) {
    return std::make_unique<LinuxCSDCustomFrameLayout>(window);
  } else {
    return std::make_unique<LinuxCSDNativeFrameLayout>(window);
  }
}

gfx::Insets LinuxFrameLayout::GetResizeBorderInsets() const {
  gfx::Insets insets = RestoredFrameBorderInsets();
  return insets.IsEmpty() ? GetInputInsets() : insets;
}

SkRRect LinuxFrameLayout::GetRoundedWindowBounds() const {
  SkRect rect = gfx::RectToSkRect(GetWindowBounds());
  SkRRect rrect;
  float radius = GetTopCornerRadiusDip();
  if (radius > 0) {
    SkPoint round_point{radius, radius};
    SkPoint radii[] = {round_point, round_point, {}, {}};
    rrect.setRectRadii(rect, radii);
  } else {
    rrect.setRect(rect);
  }
  return rrect;
}

// Base implementation is suitable for X11/views without shadows
LinuxFrameLayout::LinuxFrameLayout(NativeWindowViews* window)
    : window_(window) {
  host_supports_client_frame_shadow_ = false;
}

LinuxFrameLayout::~LinuxFrameLayout() = default;

gfx::Insets LinuxFrameLayout::RestoredFrameBorderInsets() const {
  return gfx::Insets();
}

gfx::Insets LinuxFrameLayout::FrameBorderInsets(bool restored) const {
  return !restored && (window_->IsMaximized() || window_->IsFullscreen())
             ? gfx::Insets()
             : RestoredFrameBorderInsets();
}

gfx::Insets LinuxFrameLayout::GetInputInsets() const {
  return gfx::Insets(kResizeInsideBoundsSize);
}

bool LinuxFrameLayout::IsShowingShadow() const {
  return host_supports_client_frame_shadow_ && !window_->IsMaximized() &&
         !window_->IsFullscreen();
}

bool LinuxFrameLayout::SupportsClientFrameShadow() const {
  return host_supports_client_frame_shadow_;
}

bool LinuxFrameLayout::tiled() const {
  return tiled_;
}

void LinuxFrameLayout::set_tiled(bool tiled) {
  tiled_ = tiled;
}

gfx::Rect LinuxFrameLayout::GetWindowBounds() const {
  gfx::Rect bounds = window_->widget()->GetWindowBoundsInScreen();
  bounds.Inset(FrameBorderInsets(false));
  return bounds;
}

float LinuxFrameLayout::GetTopCornerRadiusDip() const {
  return 0;
}

int LinuxFrameLayout::GetTranslucentTopAreaHeight() const {
  return 0;
}

gfx::Insets LinuxFrameLayout::NormalizeBorderInsets(
    const gfx::Insets& frame_insets,
    const gfx::Insets& input_insets) const {
  auto expand_if_visible = [](int side_thickness, int min_band) {
    return side_thickness > 0 ? std::max(side_thickness, min_band) : 0;
  };

  // Ensure hit testing for resize targets works
  // even if borders/shadows are absent on some edges.
  gfx::Insets merged;
  merged.set_top(expand_if_visible(frame_insets.top(), input_insets.top()));
  merged.set_left(expand_if_visible(frame_insets.left(), input_insets.left()));
  merged.set_bottom(
      expand_if_visible(frame_insets.bottom(), input_insets.bottom()));
  merged.set_right(
      expand_if_visible(frame_insets.right(), input_insets.right()));

  return base::i18n::IsRTL() ? gfx::Insets::TLBR(merged.top(), merged.right(),
                                                 merged.bottom(), merged.left())
                             : merged;
}

// Used for a native-like frame with a FrameProvider
LinuxCSDNativeFrameLayout::LinuxCSDNativeFrameLayout(NativeWindowViews* window)
    : LinuxFrameLayout(window) {
  host_supports_client_frame_shadow_ = CheckClientFrameShadowSupport(window);
}

LinuxCSDNativeFrameLayout::~LinuxCSDNativeFrameLayout() = default;

gfx::Insets LinuxCSDNativeFrameLayout::RestoredFrameBorderInsets() const {
  const gfx::Insets input_insets = GetInputInsets();
  const gfx::Insets frame_insets = GetFrameProvider()->GetFrameThicknessDip();
  return NormalizeBorderInsets(frame_insets, input_insets);
}

gfx::Insets LinuxCSDNativeFrameLayout::GetInputInsets() const {
  return gfx::Insets(IsShowingShadow() ? kResizeBorder : 0);
}

float LinuxCSDNativeFrameLayout::GetTopCornerRadiusDip() const {
  return window_->IsMaximized() ? 0
                                : GetFrameProvider()->GetTopCornerRadiusDip();
}

ui::WindowFrameProvider* LinuxCSDNativeFrameLayout::GetFrameProvider() const {
  return ui::LinuxUiTheme::GetForProfile(nullptr)->GetWindowFrameProvider(
      !host_supports_client_frame_shadow_, tiled(), window_->IsMaximized());
}

// Used for Chromium-like custom CSD
LinuxCSDCustomFrameLayout::LinuxCSDCustomFrameLayout(NativeWindowViews* window)
    : LinuxFrameLayout(window) {
  host_supports_client_frame_shadow_ = CheckClientFrameShadowSupport(window);
}

LinuxCSDCustomFrameLayout::~LinuxCSDCustomFrameLayout() = default;

gfx::Insets LinuxCSDCustomFrameLayout::RestoredFrameBorderInsets() const {
  const gfx::Insets input_insets = GetInputInsets();
  const bool showing_shadow = IsShowingShadow();
  const auto shadow_values = (showing_shadow && !tiled())
                                 ? GetFrameShadowValuesLinux(/*active=*/true)
                                 : gfx::ShadowValues();
  const gfx::Insets frame_insets = GetRestoredFrameBorderInsetsLinux(
      showing_shadow, kDefaultCustomFrameBorder, shadow_values, input_insets);
  return NormalizeBorderInsets(frame_insets, input_insets);
}

gfx::Insets LinuxCSDCustomFrameLayout::GetInputInsets() const {
  return gfx::Insets(IsShowingShadow() ? kResizeBorder : 0);
}

gfx::ShadowValues GetFrameShadowValuesLinux(bool active) {
  const int elevation = views::LayoutProvider::Get()->GetShadowElevationMetric(
      active ? views::Emphasis::kMaximum : views::Emphasis::kMedium);
  return gfx::ShadowValue::MakeMdShadowValues(elevation);
}

}  // namespace electron
