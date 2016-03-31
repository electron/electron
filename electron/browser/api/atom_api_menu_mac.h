// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_API_ATOM_API_MENU_MAC_H_
#define ELECTRON_BROWSER_API_ATOM_API_MENU_MAC_H_

#include "electron/browser/api/atom_api_menu.h"

#include <string>

#import "electron/browser/ui/cocoa/atom_menu_controller.h"

namespace atom {

namespace api {

class MenuMac : public Menu {
 protected:
  MenuMac();

  void PopupAt(Window* window, int x, int y, int positioning_item = 0) override;

  base::scoped_nsobject<AtomMenuController> menu_controller_;

 private:
  friend class Menu;

  static void SendActionToFirstResponder(const std::string& action);

  DISALLOW_COPY_AND_ASSIGN(MenuMac);
};

}  // namespace api

}  // namespace atom

#endif  // ELECTRON_BROWSER_API_ATOM_API_MENU_MAC_H_
