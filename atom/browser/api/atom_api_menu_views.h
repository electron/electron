// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_

#include "atom/browser/api/atom_api_menu.h"

namespace views {
class MenuRunner;
}

namespace atom {

namespace api {

class MenuViews : public Menu {
 public:
  MenuViews();

 protected:
  virtual void Popup(Window* window) OVERRIDE;

 private:
  scoped_ptr<views::MenuRunner> menu_runner_;

  DISALLOW_COPY_AND_ASSIGN(MenuViews);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_
