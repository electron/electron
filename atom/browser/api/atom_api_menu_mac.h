// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "atom/browser/api/atom_api_menu.h"

#include <string>

#import "atom/browser/ui/cocoa/atom_menu_controller.h"

namespace atom {

namespace api {

class MenuMac : public Menu {
 protected:
  MenuMac(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

  void PopupAt(Window* window, int x, int y, int positioning_item) override;

  base::scoped_nsobject<AtomMenuController> menu_controller_;

 private:
  friend class Menu;

  static void SendActionToFirstResponder(const std::string& action);

  DISALLOW_COPY_AND_ASSIGN(MenuMac);
};

}  // namespace api

}  // namespace atom
