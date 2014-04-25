// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/menu_2.h"

#include "ui/base/models/menu_model.h"
#include "ui/views/controls/menu/menu_listener.h"

// Really bad hack here, renaming all class names would be too much work.
using namespace views;  // NOLINT

namespace atom {

Menu2::Menu2(ui::MenuModel* model, bool as_window_menu)
    : model_(model),
      wrapper_(new NativeMenuWin(model, NULL)) {
  wrapper_->set_create_as_window_menu(as_window_menu);
  Rebuild();
}

Menu2::~Menu2() {}

HMENU Menu2::GetNativeMenu() const {
  return wrapper_->GetNativeMenu();
}

void Menu2::RunMenuAt(const gfx::Point& point, Alignment alignment) {
  wrapper_->RunMenuAt(point, alignment);
}

void Menu2::RunContextMenuAt(const gfx::Point& point) {
  RunMenuAt(point, ALIGN_TOPLEFT);
}

void Menu2::CancelMenu() {
  wrapper_->CancelMenu();
}

void Menu2::Rebuild() {
  wrapper_->Rebuild(NULL);
}

void Menu2::UpdateStates() {
  wrapper_->UpdateStates();
}

NativeMenuWin::MenuAction Menu2::GetMenuAction() const {
  return wrapper_->GetMenuAction();
}

void Menu2::AddMenuListener(MenuListener* listener) {
  wrapper_->AddMenuListener(listener);
}

void Menu2::RemoveMenuListener(MenuListener* listener) {
  wrapper_->RemoveMenuListener(listener);
}

void Menu2::SetMinimumWidth(int width) {
  wrapper_->SetMinimumWidth(width);
}

}  // namespace atom
