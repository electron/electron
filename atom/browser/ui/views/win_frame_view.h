// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_
#define ATOM_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_

#include "ui/views/window/non_client_view.h"

namespace views {
class Widget;
}

namespace atom {

class WinFrameView : public views::NonClientFrameView {
 public:
  explicit WinFrameView(views::Widget* widget);
  virtual ~WinFrameView();

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

  // views::View:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual gfx::Size GetMinimumSize() OVERRIDE;
  virtual gfx::Size GetMaximumSize() OVERRIDE;
  virtual const char* GetClassName() const OVERRIDE;

 private:
  void ClientAreaSizeToWindowSize(gfx::Size* size) const;

  // Our containing frame.
  views::Widget* frame_;

  DISALLOW_COPY_AND_ASSIGN(WinFrameView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_
