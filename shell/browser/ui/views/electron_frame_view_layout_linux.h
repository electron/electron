// Copyright (c) 2026 Athul Iddya.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LAYOUT_LINUX_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LAYOUT_LINUX_H_

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/window/frame_view_layout_linux.h"

namespace electron {

class NativeWindowViews;

// Adapts FrameViewLayoutLinux for ElectronFrameViewLinux's shadow-only, WCO,
// and transparent modes.
class ElectronFrameViewLayoutLinux : public views::FrameViewLayoutLinux {
 public:
  explicit ElectronFrameViewLayoutLinux(NativeWindowViews* window);
  ~ElectronFrameViewLayoutLinux() override;

  bool wants_frame() const { return wants_frame_; }
  void set_wants_frame(bool wants_frame) { wants_frame_ = wants_frame; }

  // Returns the titlebar region between leading and trailing caption buttons.
  gfx::Rect GetWindowControlsOverlayRect() const;

  // Returns the titlebar regions occupied by leading and trailing buttons.
  gfx::Rect GetLeadingButtonRect() const;
  gfx::Rect GetTrailingButtonRect() const;

  // FrameViewLayoutLinux:
  gfx::Insets GetRestoredFrameBorderInsets() const override;
  int GetTopAreaHeight() const override;
  int GetClientTopOffset() const override;
  bool ShouldShowTitlebarAndBorder() const override;
  gfx::ShadowValues GetShadowValues(bool active) const override;
  gfx::RoundedCornersF GetCornerRadii() const override;

 protected:
  // FrameViewLayoutLinux:
  void LayoutWindowControls() override;

 private:
  int GetWCOContentHeight() const;

  raw_ptr<NativeWindowViews> window_;
  bool wants_frame_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LAYOUT_LINUX_H_
