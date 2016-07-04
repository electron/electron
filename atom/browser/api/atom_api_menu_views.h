// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_

#include "atom/browser/api/atom_api_menu.h"
#include "ui/display/screen.h"

namespace atom {

namespace api {

class MenuViews : public Menu {
 public:
  explicit MenuViews(v8::Isolate* isolate);

 protected:
  void PopupAt(Window* window, int x, int y, int positioning_item) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MenuViews);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_
