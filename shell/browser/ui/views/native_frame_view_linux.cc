// Copyright (c) 2026 Athul Iddya.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/native_frame_view_linux.h"

#include <utility>

#include "shell/browser/native_window_views.h"
#include "ui/base/hit_test.h"
#include "ui/base/metadata/metadata_impl_macros.h"

namespace electron {

NativeFrameViewLinux::NativeFrameViewLinux(
    NativeWindowViews* window,
    views::Widget* widget,
    std::unique_ptr<ui::NavButtonProvider> nav_button_provider,
    views::NativeFrameViewLayoutLinux::FrameProviderGetter getter)
    : views::NativeFrameViewLinux(widget,
                                  std::move(nav_button_provider),
                                  std::move(getter)),
      window_(window) {
  InitViews();
}

NativeFrameViewLinux::~NativeFrameViewLinux() = default;

int NativeFrameViewLinux::NonClientHitTest(const gfx::Point& point) {
  int result = views::NativeFrameViewLinux::NonClientHitTest(point);
  if (result != HTCLIENT)
    return result;

  int contents_hit_test = window_->NonClientHitTest(point);
  if (contents_hit_test != HTNOWHERE)
    return contents_hit_test;

  return HTCLIENT;
}

gfx::Size NativeFrameViewLinux::GetMinimumSize() const {
  return window_->GetMinimumSize();
}

gfx::Size NativeFrameViewLinux::GetMaximumSize() const {
  return window_->GetMaximumSize();
}

BEGIN_METADATA(NativeFrameViewLinux)
END_METADATA

}  // namespace electron
