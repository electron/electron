// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_
#define ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_

#include "ui/views/view.h"

namespace atom {

class MenuBar : public views::View {
 public:
  MenuBar();
  virtual ~MenuBar();

 protected:
  // views::View:
  virtual void Paint(gfx::Canvas* canvas) OVERRIDE;
  virtual const char* GetClassName() const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(MenuBar);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_
