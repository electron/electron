// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
//
// Portions of this file are sourced from
// chrome/browser/ui/views/frame/glass_browser_frame_view.h,
// Copyright (c) 2012 The Chromium Authors,
// which is governed by a BSD-style license

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_

#include "shell/browser/ui/views/frameless_view.h"
#include "shell/browser/ui/views/win_caption_button.h"
#include "shell/browser/ui/views/win_caption_button_container.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace electron {

class NativeWindowViews;

class WinFrameView : public FramelessView {
  METADATA_HEADER(WinFrameView, FramelessView)

 public:
  WinFrameView();
  ~WinFrameView() override;

  void Init(NativeWindowViews* window, views::Widget* frame) override;
  void InvalidateCaptionButtons() override;

  SkColor GetReadableFeatureColor(SkColor background_color);

  // views::NonClientFrameView:
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;

  WinCaptionButtonContainer* caption_button_container() {
    return caption_button_container_;
  }

  bool IsMaximized() const;

  // Visual height of the titlebar when the window is maximized (i.e. excluding
  // the area above the top of the screen).
  int TitlebarMaximizedVisualHeight() const;

 protected:
  // views::View:
  void Layout(PassKey) override;

 private:
  friend class WinCaptionButtonContainer;

  int FrameBorderThickness() const;

  // views::ViewTargeterDelegate:
  views::View* TargetForRect(views::View* root, const gfx::Rect& rect) override;

  // Returns the thickness of the window border for the top edge of the frame,
  // which is sometimes different than FrameBorderThickness(). Does not include
  // the titlebar/tabstrip area. If |restored| is true, this is calculated as if
  // the window was restored, regardless of its current state.
  int FrameTopBorderThickness(bool restored) const;
  int FrameTopBorderThicknessPx(bool restored) const;

  // Returns the height of the titlebar for popups or other browser types that
  // don't have tabs.
  int TitlebarHeight(int custom_height) const;

  // Returns the y coordinate for the top of the frame, which in maximized mode
  // is the top of the screen and in restored mode is 1 pixel below the top of
  // the window to leave room for the visual border that Windows draws.
  int WindowTopY() const;

  void LayoutCaptionButtons();
  void LayoutWindowControlsOverlay();

  // The container holding the caption buttons (minimize, maximize, close, etc.)
  // May be null if the caption button container is destroyed before the frame
  // view. Always check for validity before using!
  raw_ptr<WinCaptionButtonContainer> caption_button_container_ = nullptr;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_
