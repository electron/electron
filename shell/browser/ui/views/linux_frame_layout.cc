// Copyright (c) 2025 Mitchell Cohen.
// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/linux_frame_layout.h"
#include "base/i18n/rtl.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/electron_desktop_window_tree_host_linux.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/linux/linux_ui.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/widget/widget.h"

namespace electron {

namespace {
// This should match Chromium's value.
constexpr int kResizeBorder = 10;
// This should match FramelessView's inside resize band.
constexpr int kResizeInsideBoundsSize = 5;
}  // namespace

// static
std::unique_ptr<LinuxFrameLayout> LinuxFrameLayout::Create(
    NativeWindowViews* window,
    bool wants_shadow) {
  if (x11_util::IsX11() || window->IsTranslucent() || !wants_shadow) {
    return std::make_unique<LinuxUndecoratedFrameLayout>(window);
  } else {
    return std::make_unique<LinuxCSDFrameLayout>(window);
  }
}

LinuxCSDFrameLayout::LinuxCSDFrameLayout(NativeWindowViews* window)
    : window_(window) {
  host_supports_client_frame_shadow_ = SupportsClientFrameShadow();
}

bool LinuxCSDFrameLayout::tiled() const {
  return tiled_;
}

void LinuxCSDFrameLayout::set_tiled(bool tiled) {
  tiled_ = tiled;
}

gfx::Insets LinuxCSDFrameLayout::RestoredFrameBorderInsets() const {
  gfx::Insets insets = GetFrameProvider()->GetFrameThicknessDip();
  const gfx::Insets input = GetInputInsets();

  auto expand_if_visible = [](int side_thickness, int min_band) {
    return side_thickness > 0 ? std::max(side_thickness, min_band) : 0;
  };

  gfx::Insets merged;
  merged.set_top(expand_if_visible(insets.top(), input.top()));
  merged.set_left(expand_if_visible(insets.left(), input.left()));
  merged.set_bottom(expand_if_visible(insets.bottom(), input.bottom()));
  merged.set_right(expand_if_visible(insets.right(), input.right()));

  return base::i18n::IsRTL() ? gfx::Insets::TLBR(merged.top(), merged.right(),
                                                 merged.bottom(), merged.left())
                             : merged;
}

gfx::Insets LinuxCSDFrameLayout::GetInputInsets() const {
  bool showing_shadow = host_supports_client_frame_shadow_ &&
                        !window_->IsMaximized() && !window_->IsFullscreen();
  return gfx::Insets(showing_shadow ? kResizeBorder : 0);
}

bool LinuxCSDFrameLayout::SupportsClientFrameShadow() const {
  auto* tree_host = static_cast<ElectronDesktopWindowTreeHostLinux*>(
      ElectronDesktopWindowTreeHostLinux::GetHostForWidget(
          window_->GetAcceleratedWidget()));
  return tree_host->SupportsClientFrameShadow();
}

void LinuxCSDFrameLayout::PaintWindowFrame(gfx::Canvas* canvas,
                                           gfx::Rect local_bounds,
                                           gfx::Rect titlebar_bounds,
                                           bool active) {
  GetFrameProvider()->PaintWindowFrame(
      canvas, local_bounds, titlebar_bounds.bottom(), active, GetInputInsets());
}

gfx::Rect LinuxCSDFrameLayout::GetWindowContentBounds() const {
  gfx::Rect content_bounds = window_->widget()->GetWindowBoundsInScreen();
  content_bounds.Inset(RestoredFrameBorderInsets());
  return content_bounds;
}

SkRRect LinuxCSDFrameLayout::GetRoundedWindowContentBounds() const {
  SkRect rect = gfx::RectToSkRect(GetWindowContentBounds());
  SkRRect rrect;

  if (!window_->IsMaximized()) {
    float radius = GetFrameProvider()->GetTopCornerRadiusDip();
    SkPoint round_point{radius, radius};
    SkPoint radii[] = {round_point, round_point, {}, {}};
    rrect.setRectRadii(rect, radii);
  } else {
    rrect.setRect(rect);
  }

  return rrect;
}

int LinuxCSDFrameLayout::GetTranslucentTopAreaHeight() const {
  return 0;
}

ui::WindowFrameProvider* LinuxCSDFrameLayout::GetFrameProvider() const {
  return ui::LinuxUiTheme::GetForProfile(nullptr)->GetWindowFrameProvider(
      !host_supports_client_frame_shadow_, tiled(), window_->IsMaximized());
}

LinuxUndecoratedFrameLayout::LinuxUndecoratedFrameLayout(
    NativeWindowViews* window)
    : window_(window) {}

gfx::Insets LinuxUndecoratedFrameLayout::RestoredFrameBorderInsets() const {
  return gfx::Insets();
}

gfx::Insets LinuxUndecoratedFrameLayout::GetInputInsets() const {
  return gfx::Insets(kResizeInsideBoundsSize);
}

bool LinuxUndecoratedFrameLayout::SupportsClientFrameShadow() const {
  return false;
}

bool LinuxUndecoratedFrameLayout::tiled() const {
  return tiled_;
}

void LinuxUndecoratedFrameLayout::set_tiled(bool tiled) {
  tiled_ = tiled;
}

void LinuxUndecoratedFrameLayout::PaintWindowFrame(gfx::Canvas* canvas,
                                                   gfx::Rect local_bounds,
                                                   gfx::Rect titlebar_bounds,
                                                   bool active) {
  // No-op
}

gfx::Rect LinuxUndecoratedFrameLayout::GetWindowContentBounds() const {
  // With no transparent insets, widget bounds and logical bounds match.
  return window_->widget()->GetWindowBoundsInScreen();
}

SkRRect LinuxUndecoratedFrameLayout::GetRoundedWindowContentBounds() const {
  SkRRect rrect;
  rrect.setRect(gfx::RectToSkRect(GetWindowContentBounds()));
  return rrect;
}

int LinuxUndecoratedFrameLayout::GetTranslucentTopAreaHeight() const {
  return 0;
}

ui::WindowFrameProvider* LinuxUndecoratedFrameLayout::GetFrameProvider() const {
  return nullptr;
}

}  // namespace electron
