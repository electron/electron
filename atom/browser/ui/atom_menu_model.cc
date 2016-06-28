// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/atom_menu_model.h"

#include "atom/browser/ui/menu_model_context.h"
#include "base/stl_util.h"

namespace atom {

AtomMenuModel::AtomMenuModel(MenuModelDelegate* delegate)
    : ui::SimpleMenuModel(delegate),
      delegate_(delegate) {
}

AtomMenuModel::~AtomMenuModel() {
}

void AtomMenuModel::SetRole(int index, const base::string16& role) {
  int command_id = GetCommandIdAt(index);
  roles_[command_id] = role;
}

base::string16 AtomMenuModel::GetRoleAt(int index) {
  int command_id = GetCommandIdAt(index);
  if (ContainsKey(roles_, command_id))
    return roles_[command_id];
  else
    return base::string16();
}

ui::MenuModel* AtomMenuModel::GetTrayModel() {
  if (!tray_model_) {
    tray_model_.reset(new MenuModelContext("tray", this));
  }
  return tray_model_.get();
}


void AtomMenuModel::MenuClosed() {
  ui::SimpleMenuModel::MenuClosed();
  FOR_EACH_OBSERVER(Observer, observers_, MenuClosed());
}

}  // namespace atom
