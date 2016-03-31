// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/browser/ui/electron_menu_model.h"

#include "base/stl_util.h"

namespace electron {

ElectronMenuModel::ElectronMenuModel(Delegate* delegate)
    : ui::SimpleMenuModel(delegate),
      delegate_(delegate) {
}

ElectronMenuModel::~ElectronMenuModel() {
}

void ElectronMenuModel::SetRole(int index, const base::string16& role) {
  roles_[index] = role;
}

base::string16 ElectronMenuModel::GetRoleAt(int index) {
  if (ContainsKey(roles_, index))
    return roles_[index];
  else
    return base::string16();
}

void ElectronMenuModel::MenuClosed() {
  ui::SimpleMenuModel::MenuClosed();
  FOR_EACH_OBSERVER(Observer, observers_, MenuClosed());
}

}  // namespace electron
