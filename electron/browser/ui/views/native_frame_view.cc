// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/native_frame_view.h"

#include "atom/browser/native_window.h"

namespace atom {

namespace {

const char kViewClassName[] = "AtomNativeFrameView";

}  // namespace

NativeFrameView::NativeFrameView(NativeWindow* window, views::Widget* widget)
    : views::NativeFrameView(widget),
      window_(window) {
}

gfx::Size NativeFrameView::GetMinimumSize() const {
  return window_->GetMinimumSize();
}

gfx::Size NativeFrameView::GetMaximumSize() const {
  return window_->GetMaximumSize();
}

const char* NativeFrameView::GetClassName() const {
  return kViewClassName;
}

}  // namespace atom
