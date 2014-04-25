// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_menu_win.h"

#include "atom/browser/native_window_win.h"
#include "atom/browser/ui/win/menu_2.h"
#include "ui/gfx/point.h"
#include "ui/gfx/screen.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

MenuWin::MenuWin() {
}

void MenuWin::Popup(Window* window) {
  gfx::Point cursor = gfx::Screen::GetNativeScreen()->GetCursorScreenPoint();
  menu_.reset(new atom::Menu2(model_.get()));
  menu_->RunContextMenuAt(cursor);
}

void Menu::AttachToWindow(Window* window) {
  static_cast<NativeWindowWin*>(window->window())->SetMenu(model_.get());
}

// static
mate::Wrappable* Menu::Create() {
  return new MenuWin();
}

}  // namespace api

}  // namespace atom
