// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_
#define ATOM_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_

#include "ui/views/window/non_client_view.h"

namespace views {
class Widget;
}

namespace atom {

class NativeWindowViews;

class FramelessView : public views::NonClientFrameView {
 public:
  FramelessView();
  virtual ~FramelessView();

  virtual void Init(NativeWindowViews* window, views::Widget* frame);

  // Returns whether the |point| is on frameless window's resizing border.
  int ResizingBorderHitTest(const gfx::Point& point);

 protected:
  // views::NonClientFrameView:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void GetWindowMask(const gfx::Size& size,
                     gfx::Path* window_mask) override;
  void ResetWindowControls() override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;
  void SizeConstraintsChanged() override;

  // Overridden from View:
  gfx::Size GetPreferredSize() const override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  const char* GetClassName() const override;

  // Not owned.
  NativeWindowViews* window_;
  views::Widget* frame_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FramelessView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_
