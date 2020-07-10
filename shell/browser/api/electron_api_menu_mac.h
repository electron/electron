// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_MENU_MAC_H_
#define SHELL_BROWSER_API_ELECTRON_API_MENU_MAC_H_

#include "shell/browser/api/electron_api_menu.h"

#include <map>
#include <string>

#import "shell/browser/ui/cocoa/electron_menu_controller.h"

using base::scoped_nsobject;

namespace electron {

namespace api {

class MenuMac : public Menu {
 protected:
  explicit MenuMac(gin::Arguments* args);
  ~MenuMac() override;

  void PopupAt(BaseWindow* window,
               int x,
               int y,
               int positioning_item,
               base::OnceClosure callback) override;
  void PopupOnUI(const base::WeakPtr<NativeWindow>& native_window,
                 int32_t window_id,
                 int x,
                 int y,
                 int positioning_item,
                 base::OnceClosure callback);
  void ClosePopupAt(int32_t window_id) override;
  void ClosePopupOnUI(int32_t window_id);

 private:
  friend class Menu;

  void OnClosed(int32_t window_id, base::OnceClosure callback);

  scoped_nsobject<ElectronMenuController> menu_controller_;

  // window ID -> open context menu
  std::map<int32_t, scoped_nsobject<ElectronMenuController>> popup_controllers_;

  base::WeakPtrFactory<MenuMac> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MenuMac);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_MENU_MAC_H_
