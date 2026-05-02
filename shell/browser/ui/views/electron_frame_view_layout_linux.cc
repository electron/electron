// Copyright (c) 2026 Athul Iddya.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/electron_frame_view_layout_linux.h"

#include "shell/browser/native_window_views.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/caption_button_layout_constants.h"
#include "ui/views/window/frame_view_linux.h"

namespace electron {

ElectronFrameViewLayoutLinux::ElectronFrameViewLayoutLinux(
    NativeWindowViews* window)
    : window_(window),
      wants_frame_(
          !window->IsTranslucent() &&
          (window->HasShadow() || window->IsWindowControlsOverlayEnabled())) {}

ElectronFrameViewLayoutLinux::~ElectronFrameViewLayoutLinux() = default;

gfx::Insets ElectronFrameViewLayoutLinux::GetRestoredFrameBorderInsets() const {
  if (window_->IsTranslucent())
    return gfx::Insets();

  return FrameViewLayoutLinux::GetRestoredFrameBorderInsets();
}

int ElectronFrameViewLayoutLinux::GetWCOContentHeight() const {
  int custom_height = window_->titlebar_overlay_height();
  if (custom_height)
    return custom_height;

  return views::GetCaptionButtonLayoutSize(
             views::CaptionButtonLayoutSize::kNonBrowserCaption)
      .height();
}

int ElectronFrameViewLayoutLinux::GetTopAreaHeight() const {
  if (!ShouldShowTitlebarAndBorder())
    return 0;

  int border_top = GetFrameBorderInsets().top();
  return border_top + (window_->IsWindowControlsOverlayEnabled()
                           ? GetWCOContentHeight()
                           : 0);
}

int ElectronFrameViewLayoutLinux::GetClientTopOffset() const {
  // Content starts at the frame border; WCO buttons overlay via layers.
  return GetFrameBorderInsets().top();
}

gfx::ShadowValues ElectronFrameViewLayoutLinux::GetShadowValues(
    bool active) const {
  if (!window_->HasShadow())
    return gfx::ShadowValues();
  return FrameViewLayoutLinux::GetShadowValues(active);
}

gfx::RoundedCornersF ElectronFrameViewLayoutLinux::GetCornerRadii() const {
  // TODO: implement client-area clipping for rounded corners.
  return gfx::RoundedCornersF();
}

bool ElectronFrameViewLayoutLinux::ShouldShowTitlebarAndBorder() const {
  if (window_->IsTranslucent())
    return false;
  return !view()->GetWidget()->IsFullscreen();
}

gfx::Rect ElectronFrameViewLayoutLinux::GetWindowControlsOverlayRect() const {
  // Convert frame-view titlebar coordinates to client-view-relative.
  gfx::Insets insets = GetFrameBorderInsets();
  int client_left = insets.left();
  int client_right = view()->width() - insets.right();
  int x = std::max(0, minimum_titlebar_x_ - client_left);
  int right = std::min(client_right, maximum_titlebar_x_) - client_left;
  int height = GetTopAreaHeight() - GetClientTopOffset();
  return gfx::Rect(x, 0, std::max(0, right - x), height);
}

gfx::Rect ElectronFrameViewLayoutLinux::GetLeadingButtonRect() const {
  gfx::Insets insets = GetFrameBorderInsets();
  int x = insets.left();
  if (minimum_titlebar_x_ <= x)
    return gfx::Rect();
  int y = GetClientTopOffset();
  int height = GetTopAreaHeight() - y;
  return gfx::Rect(x, y, minimum_titlebar_x_ - x, height);
}

gfx::Rect ElectronFrameViewLayoutLinux::GetTrailingButtonRect() const {
  gfx::Insets insets = GetFrameBorderInsets();
  int right = view()->width() - insets.right();
  if (maximum_titlebar_x_ >= right)
    return gfx::Rect();
  int y = GetClientTopOffset();
  int height = GetTopAreaHeight() - y;
  return gfx::Rect(maximum_titlebar_x_, y, right - maximum_titlebar_x_, height);
}

void ElectronFrameViewLayoutLinux::LayoutWindowControls() {
  if (!window_->IsWindowControlsOverlayEnabled())
    return;

  views::FrameViewLayoutLinux::LayoutWindowControls();
}

}  // namespace electron
