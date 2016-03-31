// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_API_ATOM_API_MENU_VIEWS_H_
#define ELECTRON_BROWSER_API_ATOM_API_MENU_VIEWS_H_

#include "electron/browser/api/atom_api_menu.h"
#include "ui/gfx/screen.h"

namespace atom {

namespace api {

class MenuViews : public Menu {
 public:
  MenuViews();

 protected:
  void PopupAt(Window* window, int x, int y, int positioning_item = 0) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MenuViews);
};

}  // namespace api

}  // namespace atom

#endif  // ELECTRON_BROWSER_API_ATOM_API_MENU_VIEWS_H_
