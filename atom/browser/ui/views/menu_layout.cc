// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_layout.h"

namespace atom {

MenuLayout::MenuLayout(int menu_height)
    : menu_height_(menu_height) {
}

MenuLayout::~MenuLayout() {
}

void MenuLayout::Layout(views::View* host) {
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

gfx::Size MenuLayout::GetPreferredSize(views::View* host) {
  gfx::Size size = views::FillLayout::GetPreferredSize(host);
  if (!HasMenu(host))
    return size;

  size.set_height(size.height() + menu_height_);
  return size;
}

int MenuLayout::GetPreferredHeightForWidth(views::View* host, int width) {
  int height = views::FillLayout::GetPreferredHeightForWidth(host, width);
  if (!HasMenu(host))
    return height;

  return height + menu_height_;
}

bool MenuLayout::HasMenu(const views::View* host) const {
  return host->child_count() == 2;
}

}  // namespace atom
