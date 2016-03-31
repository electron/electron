// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_API_ELECTRON_API_MENU_MAC_H_
#define ELECTRON_BROWSER_API_ELECTRON_API_MENU_MAC_H_

#include "electron/browser/api/electron_api_menu.h"

#include <string>

#import "electron/browser/ui/cocoa/electron_menu_controller.h"

namespace electron {

namespace api {

class MenuMac : public Menu {
 protected:
  MenuMac();

  void PopupAt(Window* window, int x, int y, int positioning_item = 0) override;

  base::scoped_nsobject<ElectronMenuController> menu_controller_;

 private:
  friend class Menu;

  static void SendActionToFirstResponder(const std::string& action);

  DISALLOW_COPY_AND_ASSIGN(MenuMac);
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_BROWSER_API_ELECTRON_API_MENU_MAC_H_
