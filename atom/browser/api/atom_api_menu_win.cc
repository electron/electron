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

MenuWin::MenuWin() : menu_(NULL) {
}

void MenuWin::Popup(Window* window) {
  gfx::Point cursor = gfx::Screen::GetNativeScreen()->GetCursorScreenPoint();
  popup_menu_.reset(new atom::Menu2(model_.get()));
  menu_ = popup_menu_.get();
  menu_->RunContextMenuAt(cursor);
}

void MenuWin::UpdateStates() {
  MenuWin* top = this;
  while (top->parent_)
    top = static_cast<MenuWin*>(top->parent_);
  if (top->menu_)
    top->menu_->UpdateStates();
}

void MenuWin::AttachToWindow(Window* window) {
  NativeWindowWin* nw = static_cast<NativeWindowWin*>(window->window());
  nw->SetMenu(model_.get());
  menu_ = nw->menu();
}

// static
mate::Wrappable* Menu::Create() {
  return new MenuWin();
}

}  // namespace api

}  // namespace atom
