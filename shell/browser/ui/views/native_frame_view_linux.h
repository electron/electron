// Copyright (c) 2026 Athul Iddya.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_NATIVE_FRAME_VIEW_LINUX_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_NATIVE_FRAME_VIEW_LINUX_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/linux/nav_button_provider.h"
#include "ui/views/window/native_frame_view_layout_linux.h"
#include "ui/views/window/native_frame_view_linux.h"

namespace electron {

class NativeWindowViews;

// Extends views::NativeFrameViewLinux for Electron integration, such as
// app-defined draggable regions and window title updates.
class NativeFrameViewLinux : public views::NativeFrameViewLinux {
  METADATA_HEADER(NativeFrameViewLinux, views::NativeFrameViewLinux)

 public:
  NativeFrameViewLinux(
      NativeWindowViews* window,
      views::Widget* widget,
      std::unique_ptr<ui::NavButtonProvider> nav_button_provider,
      views::NativeFrameViewLayoutLinux::FrameProviderGetter getter);
  ~NativeFrameViewLinux() override;

  NativeWindowViews* window() const { return window_; }

  // views::FrameViewLinux:
  int NonClientHitTest(const gfx::Point& point) override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;

 private:
  raw_ptr<NativeWindowViews> window_ = nullptr;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_NATIVE_FRAME_VIEW_LINUX_H_
