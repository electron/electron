// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_LINUX_FRAME_VIEW_H_
#define ATOM_BROWSER_UI_VIEWS_LINUX_FRAME_VIEW_H_

#include "atom/browser/ui/views/frameless_view.h"
#include "base/memory/scoped_ptr.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/window/frame_buttons.h"

namespace gfx {
class ImageSkia;
}

namespace views {
class FrameBackground;
class ImageButton;
}

namespace atom {

class LinuxFrameView : public FramelessView
                       public views::ButtonListener {
 public:
  LinuxFrameView();
  virtual ~LinuxFrameView();

  void Init(NativeWindowViews* window, views::Widget* frame) OVERRIDE;

  // Overridden from NonClientFrameView:
  virtual gfx::Rect GetBoundsForClientView() const OVERRIDE;
  virtual gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const OVERRIDE;
  virtual int NonClientHitTest(const gfx::Point& point) OVERRIDE;
  virtual void GetWindowMask(const gfx::Size& size,
                             gfx::Path* window_mask) OVERRIDE;
  virtual void ResetWindowControls() OVERRIDE;
  virtual void UpdateWindowIcon() OVERRIDE;
  virtual void UpdateWindowTitle() OVERRIDE;

  // Overridden from View:
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual gfx::Size GetMinimumSize() OVERRIDE;
  virtual gfx::Size GetMaximumSize() OVERRIDE;

  // Overridden from ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

  // Returns whether the |point| is on frameless window's resizing border.
  int ResizingBorderHitTest(const gfx::Point& point);

 private:
  // Returns the thickness of the border that makes up the window frame edges.
  // This does not include any client edge.
  int FrameBorderThickness() const;

  // Returns the thickness of the entire nonclient left, right, and bottom
  // borders, including both the window frame and any client edge.
  int NonClientBorderThickness() const;

  // Returns the height of the entire nonclient top border, including the window
  // frame, any title area, and any connected client edge.
  int NonClientTopBorderHeight() const;

  // Returns the y-coordinate of the caption buttons.
  int CaptionButtonY() const;

  // Returns the thickness of the nonclient portion of the 3D edge along the
  // bottom of the titlebar.
  int TitlebarBottomThickness() const;

  // Returns the size of the titlebar icon.  This is used even when the icon is
  // not shown, e.g. to set the titlebar height.
  int IconSize() const;

  // Returns the bounds of the titlebar icon (or where the icon would be if
  // there was one).
  gfx::Rect IconBounds() const;

  // Returns true if the title bar, caption buttons, and frame border should be
  // drawn. If false, the client view occupies the full area of this view.
  bool ShouldShowTitleBarAndBorder() const;

  // Returns true if the client edge should be drawn. This is true if
  // the window is not maximized.
  bool ShouldShowClientEdge() const;

  // Paint various sub-components of this view.
  void PaintRestoredFrameBorder(gfx::Canvas* canvas);
  void PaintMaximizedFrameBorder(gfx::Canvas* canvas);
  void PaintTitleBar(gfx::Canvas* canvas);
  void PaintRestoredClientEdge(gfx::Canvas* canvas);

  // Compute aspects of the frame needed to paint the frame background.
  SkColor GetFrameColor() const;
  const gfx::ImageSkia* GetFrameImage() const;

  // Layout various sub-components of this view.
  void LayoutWindowControls();
  void LayoutTitleBar();
  void LayoutClientView();

  // Creates, adds and returns a new window caption button (e.g, minimize,
  // maximize, restore).
  views::ImageButton* InitWindowCaptionButton(int accessibility_string_id,
                                              int normal_image_id,
                                              int hot_image_id,
                                              int pushed_image_id);

  // The bounds of the client view, in this view's coordinates.
  gfx::Rect client_view_bounds_;

  // The layout rect of the title, if visible.
  gfx::Rect title_bounds_;

  // The icon of this window. May be NULL.
  views::ImageButton* window_icon_;

  // Window caption buttons.
  views::ImageButton* minimize_button_;
  views::ImageButton* maximize_button_;
  views::ImageButton* restore_button_;
  views::ImageButton* close_button_;

  // Should maximize button be shown?
  bool should_show_maximize_button_;

  // Background painter for the window frame.
  scoped_ptr<views::FrameBackground> frame_background_;

  DISALLOW_COPY_AND_ASSIGN(LinuxFrameView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_LINUX_FRAME_VIEW_H_
