// Copyright (c) 2026 Athul Iddya.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LAYOUT_LINUX_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LAYOUT_LINUX_H_

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/window/native_frame_view_layout_linux.h"

namespace ui {
class NavButtonProvider;
}  // namespace ui

namespace electron {

class NativeWindowViews;

// Adapts NativeFrameViewLayoutLinux for ElectronFrameViewLinux's
// shadow-only, WCO, and transparent modes.
class ElectronFrameViewLayoutLinux : public views::NativeFrameViewLayoutLinux {
 public:
  ElectronFrameViewLayoutLinux(NativeWindowViews* window,
                               ui::NavButtonProvider* nav_buttons);
  ~ElectronFrameViewLayoutLinux() override;

  void set_nav_buttons(ui::NavButtonProvider* nav_buttons) {
    nav_buttons_ = nav_buttons;
    set_nav_button_provider(nav_buttons);
  }

  ui::NavButtonProvider* nav_buttons() const { return nav_buttons_; }

  bool wants_frame() const { return wants_frame_; }
  void set_wants_frame(bool wants_frame) { wants_frame_ = wants_frame; }

  // Returns the titlebar region between leading and trailing caption buttons.
  gfx::Rect GetWindowControlsOverlayRect() const;

  // Returns the titlebar regions occupied by leading and trailing buttons.
  gfx::Rect GetLeadingButtonRect() const;
  gfx::Rect GetTrailingButtonRect() const;

  // NativeFrameViewLayoutLinux:
  gfx::Insets GetRestoredFrameBorderInsets() const override;
  int GetTopAreaHeight() const override;
  int GetClientTopOffset() const override;
  bool ShouldShowTitlebarAndBorder() const override;
  gfx::ShadowValues GetShadowValues(bool active) const override;
  gfx::RoundedCornersF GetCornerRadii() const override;
  gfx::Insets GetTopAreaBorderInsets() const override;
  ButtonLayoutParams GetButtonLayoutParams(
      views::FrameButton button_id,
      views::Button* button) const override;
  gfx::Insets GetTopAreaSpacing() const override;

 protected:
  // NativeFrameViewLayoutLinux:
  void LayoutWindowControls() override;

 private:
  int GetWCOContentHeight() const;

  raw_ptr<NativeWindowViews> window_;
  raw_ptr<ui::NavButtonProvider> nav_buttons_;
  bool wants_frame_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LAYOUT_LINUX_H_
