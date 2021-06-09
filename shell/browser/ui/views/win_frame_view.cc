// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/win_frame_view.h"

#include <dwmapi.h>
#include <memory>

#include "base/win/windows_version.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/win_caption_button_container.h"
#include "ui/base/win/hwnd_metrics.h"
#include "ui/display/win/dpi.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"

namespace electron {

const char WinFrameView::kViewClassName[] = "WinFrameView";

WinFrameView::WinFrameView() = default;

WinFrameView::~WinFrameView() = default;

void WinFrameView::Init(NativeWindowViews* window, views::Widget* frame) {
  window_ = window;
  frame_ = frame;

  if (window->IsWindowControlsOverlayEnabled()) {
    caption_button_container_ =
        AddChildView(std::make_unique<WinCaptionButtonContainer>(this));
  } else {
    caption_button_container_ = nullptr;
  }
}

gfx::Rect WinFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  return views::GetWindowBoundsForClientBounds(
      static_cast<views::View*>(const_cast<WinFrameView*>(this)),
      client_bounds);
}

int WinFrameView::FrameBorderThickness() const {
  return (IsMaximized() || frame()->IsFullscreen())
             ? 0
             : display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXSIZEFRAME);
}

int WinFrameView::NonClientHitTest(const gfx::Point& point) {
  if (ShouldCustomDrawSystemTitlebar()) {
    // See if the point is within any of the window controls.
    if (caption_button_container_) {
      gfx::Point local_point = point;

      ConvertPointToTarget(parent(), caption_button_container_, &local_point);
      if (caption_button_container_->HitTestPoint(local_point)) {
        const int hit_test_result =
            caption_button_container_->NonClientHitTest(local_point);
        if (hit_test_result != HTNOWHERE)
          return hit_test_result;
      }
    }
  }

  // Use the parent class's hittest last
  return FramelessView::NonClientHitTest(point);
}

const char* WinFrameView::GetClassName() const {
  return kViewClassName;
}

bool WinFrameView::IsMaximized() const {
  return frame()->IsMaximized();
}

bool WinFrameView::ShouldCustomDrawSystemTitlebar() const {
  return window()->title_bar_style() !=
         NativeWindowViews::TitleBarStyle::kNormal;
}

void WinFrameView::Layout() {
  LayoutCaptionButtons();
  if (window()->IsWindowControlsOverlayEnabled()) {
    LayoutWindowControlsOverlay();
  }
  NonClientFrameView::Layout();
}

int WinFrameView::FrameTopBorderThickness(bool restored) const {
  // Mouse and touch locations are floored but GetSystemMetricsInDIP is rounded,
  // so we need to floor instead or else the difference will cause the hittest
  // to fail when it ought to succeed.
  return std::floor(
      FrameTopBorderThicknessPx(restored) /
      display::win::ScreenWin::GetScaleFactorForHWND(HWNDForView(this)));
}

int WinFrameView::FrameTopBorderThicknessPx(bool restored) const {
  // Distinct from FrameBorderThickness() because we can't inset the top
  // border, otherwise Windows will give us a standard titlebar.
  // For maximized windows this is not true, and the top border must be
  // inset in order to avoid overlapping the monitor above.

  // See comments in BrowserDesktopWindowTreeHostWin::GetClientAreaInsets().
  const bool needs_no_border =
      (ShouldCustomDrawSystemTitlebar() && frame()->IsMaximized()) ||
      frame()->IsFullscreen();
  if (needs_no_border && !restored)
    return 0;

  // Note that this method assumes an equal resize handle thickness on all
  // sides of the window.
  // TODO(dfried): Consider having it return a gfx::Insets object instead.
  return ui::GetFrameThickness(
      MonitorFromWindow(HWNDForView(this), MONITOR_DEFAULTTONEAREST));
}

int WinFrameView::TitlebarMaximizedVisualHeight() const {
  int maximized_height =
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYCAPTION);
  return maximized_height;
}

int WinFrameView::TitlebarHeight(bool restored) const {
  if (frame()->IsFullscreen() && !restored)
    return 0;

  return TitlebarMaximizedVisualHeight() + FrameTopBorderThickness(false);
}

int WinFrameView::WindowTopY() const {
  // The window top is SM_CYSIZEFRAME pixels when maximized (see the comment in
  // FrameTopBorderThickness()) and floor(system dsf) pixels when restored.
  // Unfortunately we can't represent either of those at hidpi without using
  // non-integral dips, so we return the closest reasonable values instead.
  if (IsMaximized())
    return FrameTopBorderThickness(false);

  return 1;
}

void WinFrameView::LayoutCaptionButtons() {
  if (!caption_button_container_)
    return;

  // Non-custom system titlebar already contains caption buttons.
  if (!ShouldCustomDrawSystemTitlebar()) {
    caption_button_container_->SetVisible(false);
    return;
  }

  caption_button_container_->SetVisible(true);

  const gfx::Size preferred_size =
      caption_button_container_->GetPreferredSize();
  int height = preferred_size.height();

  height = IsMaximized() ? TitlebarMaximizedVisualHeight()
                         : TitlebarHeight(false) - WindowTopY();

  caption_button_container_->SetBounds(width() - preferred_size.width(),
                                       WindowTopY(), preferred_size.width(),
                                       height);
}

void WinFrameView::LayoutWindowControlsOverlay() {
  int overlay_height = caption_button_container_->size().height();
  int overlay_width = caption_button_container_->size().width();
  int bounding_rect_width = width() - overlay_width;
  auto bounding_rect =
      GetMirroredRect(gfx::Rect(0, 0, bounding_rect_width, overlay_height));

  window()->SetWindowControlsOverlayRect(bounding_rect);
  window()->NotifyLayoutWindowControlsOverlay();
}

}  // namespace electron
