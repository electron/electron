// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_NATIVE_FRAME_VIEW_H_
#define ATOM_BROWSER_UI_VIEWS_NATIVE_FRAME_VIEW_H_

#include "ui/views/window/native_frame_view.h"

namespace atom {

class NativeWindow;

// Like the views::NativeFrameView, but returns the min/max size from the
// NativeWindowViews.
class NativeFrameView : public views::NativeFrameView {
 public:
  NativeFrameView(NativeWindow* window, views::Widget* widget);

 protected:
  // views::View:
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  const char* GetClassName() const override;

 private:
  NativeWindow* window_;  // weak ref.

  DISALLOW_COPY_AND_ASSIGN(NativeFrameView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_NATIVE_FRAME_VIEW_H_
