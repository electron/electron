// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_MAC_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_MAC_H_

#include "atom/browser/api/atom_api_menu.h"

#include <string>

#import "atom/browser/ui/cocoa/atom_menu_controller.h"

namespace atom {

namespace api {

class MenuMac : public Menu {
 protected:
  explicit MenuMac(v8::Isolate* isolate);

  void PopupAt(Window* window, int x, int y, int positioning_item = 0) override;

  base::scoped_nsobject<AtomMenuController> menu_controller_;

 private:
  friend class Menu;

  static void SendActionToFirstResponder(const std::string& action);

  DISALLOW_COPY_AND_ASSIGN(MenuMac);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_MAC_H_
