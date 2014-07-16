// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_MENU_LAYOUT_H_
#define ATOM_BROWSER_UI_VIEWS_MENU_LAYOUT_H_

#include "ui/views/layout/fill_layout.h"

namespace atom {

class MenuLayout : public views::FillLayout {
 public:
  explicit MenuLayout(int menu_height);
  virtual ~MenuLayout();

  // views::LayoutManager:
  virtual void Layout(views::View* host) OVERRIDE;
  virtual gfx::Size GetPreferredSize(views::View* host) OVERRIDE;
  virtual int GetPreferredHeightForWidth(views::View* host, int width) OVERRIDE;

 private:
  bool HasMenu(const views::View* host) const;

  int menu_height_;

  DISALLOW_COPY_AND_ASSIGN(MenuLayout);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_MENU_LAYOUT_H_
