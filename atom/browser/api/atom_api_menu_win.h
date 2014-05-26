// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_WIN_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_WIN_H_

#include "atom/browser/api/atom_api_menu.h"

namespace atom {

class Menu2;

namespace api {

class MenuWin : public Menu {
 public:
  MenuWin();

 protected:
  virtual void Popup(Window* window) OVERRIDE;
  virtual void UpdateStates() OVERRIDE;
  virtual void AttachToWindow(Window* window) OVERRIDE;

 private:
  atom::Menu2* menu_;  // Weak ref, could be window menu or popup menu.
  scoped_ptr<atom::Menu2> popup_menu_;

  DISALLOW_COPY_AND_ASSIGN(MenuWin);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_WIN_H_
