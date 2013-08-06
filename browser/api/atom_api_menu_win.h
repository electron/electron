// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_WIN_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_WIN_H_

#include "browser/api/atom_api_menu.h"

namespace views {
class Menu2;
}

namespace atom {

namespace api {

class MenuWin : public Menu {
 public:
  explicit MenuWin(v8::Handle<v8::Object> wrapper);
  virtual ~MenuWin();

 protected:
  virtual void Popup(NativeWindow* window) OVERRIDE;

 private:
  scoped_ptr<views::Menu2> menu_;

  DISALLOW_COPY_AND_ASSIGN(MenuWin);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_WIN_H_
