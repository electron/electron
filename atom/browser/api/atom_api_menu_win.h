// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
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

 private:
  scoped_ptr<atom::Menu2> menu_;

  DISALLOW_COPY_AND_ASSIGN(MenuWin);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_WIN_H_
