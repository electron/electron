// Copyright (c) 2014 GitHub, Inc. All rights reserved.
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
  virtual gfx::Size GetPreferredSize() const OVERRIDE;
  virtual gfx::Size GetMinimumSize() const OVERRIDE;
  virtual gfx::Size GetMaximumSize() const OVERRIDE;
  virtual const char* GetClassName() const OVERRIDE;

  // Not owned.
  NativeWindowViews* window_;
  views::Widget* frame_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FramelessView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_
