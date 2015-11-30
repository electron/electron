// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_
#define ATOM_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_

#include "atom/browser/ui/views/frameless_view.h"

namespace atom {

class WinFrameView : public FramelessView {
 public:
  WinFrameView();
  virtual ~WinFrameView();

  // views::NonClientFrameView:
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;

  // views::View:
  const char* GetClassName() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WinFrameView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_
