// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_API_ELECTRON_API_MENU_VIEWS_H_
#define ELECTRON_BROWSER_API_ELECTRON_API_MENU_VIEWS_H_

#include "electron/browser/api/electron_api_menu.h"
#include "ui/gfx/screen.h"

namespace electron {

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

}  // namespace electron

#endif  // ELECTRON_BROWSER_API_ELECTRON_API_MENU_VIEWS_H_
