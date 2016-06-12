// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "ui/views/layout/fill_layout.h"

namespace atom {

class NativeWindowViews;

class MenuLayout : public views::FillLayout {
 public:
  MenuLayout(NativeWindowViews* window, int menu_height);
  virtual ~MenuLayout();

  // views::LayoutManager:
  void Layout(views::View* host) override;
  gfx::Size GetPreferredSize(const views::View* host) const override;
  int GetPreferredHeightForWidth(
      const views::View* host, int width) const override;

 private:
  bool HasMenu(const views::View* host) const;

  NativeWindowViews* window_;
  int menu_height_;

  DISALLOW_COPY_AND_ASSIGN(MenuLayout);
};

}  // namespace atom
