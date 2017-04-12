// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_layout.h"

#if defined(OS_WIN)
#include <algorithm>
#include "atom/browser/native_window_views.h"
#include "ui/display/win/screen_win.h"
#endif

namespace atom {

namespace {

#if defined(OS_WIN)
gfx::Rect SubtractBorderSize(gfx::Rect bounds) {
  int frameX = display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXSIZEFRAME);
  int frameY = display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYSIZEFRAME);
  gfx::Point borderSize = gfx::Point(
      frameX,   // width
      frameY);  // height

  int diff = std::max(0, GetSystemMetrics(SM_CXSIZEFRAME) - frameX - 1);

  int basisX = borderSize.x() * 2 + diff;
  int basisY = borderSize.y() * 2 + diff;
  bounds.set_x(bounds.x() + basisX);
  bounds.set_y(bounds.y() + basisY);
  bounds.set_width(bounds.width() - basisX * 2);
  bounds.set_height(bounds.height() - basisY * 2);

  return bounds;
}
#endif

}  // namespace

MenuLayout::MenuLayout(NativeWindowViews* window, int menu_height)
    : window_(window),
      menu_height_(menu_height) {
}

MenuLayout::~MenuLayout() {
}

void MenuLayout::Layout(views::View* host) {
#if defined(OS_WIN)
  // Reserve border space for maximized frameless window so we won't have the
  // content go outside of screen.
  if (!window_->has_frame() && window_->IsMaximized()) {
    gfx::Rect bounds = SubtractBorderSize(host->GetContentsBounds());
    host->child_at(0)->SetBoundsRect(bounds);
    return;
  }
#endif

  if (!HasMenu(host)) {
    views::FillLayout::Layout(host);
    return;
  }

  gfx::Size size = host->GetContentsBounds().size();
  gfx::Rect menu_Bar_bounds = gfx::Rect(0, 0, size.width(), menu_height_);
  gfx::Rect web_view_bounds = gfx::Rect(
      0, menu_height_, size.width(), size.height() - menu_height_);

  views::View* web_view = host->child_at(0);
  views::View* menu_bar = host->child_at(1);
  web_view->SetBoundsRect(web_view_bounds);
  menu_bar->SetBoundsRect(menu_Bar_bounds);
}

gfx::Size MenuLayout::GetPreferredSize(const views::View* host) const {
  gfx::Size size = views::FillLayout::GetPreferredSize(host);
  if (!HasMenu(host))
    return size;

  size.set_height(size.height() + menu_height_);
  return size;
}

int MenuLayout::GetPreferredHeightForWidth(
    const views::View* host, int width) const {
  int height = views::FillLayout::GetPreferredHeightForWidth(host, width);
  if (!HasMenu(host))
    return height;

  return height + menu_height_;
}

bool MenuLayout::HasMenu(const views::View* host) const {
  return host->child_count() == 2;
}

}  // namespace atom
