// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_

#include "atom/browser/api/atom_api_menu.h"
#include "ui/gfx/screen.h"

namespace atom {

namespace api {

class MenuViews : public Menu {
 public:
  MenuViews();

 protected:
  void Popup(Window* window) override;
  void PopupAt(Window* window, int x, int y) override;

 private:
  void PopupAtPoint(Window* window, const gfx::Point& point);

  DISALLOW_COPY_AND_ASSIGN(MenuViews);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_
