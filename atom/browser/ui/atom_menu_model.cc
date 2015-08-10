// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/atom_menu_model.h"

namespace atom {

AtomMenuModel::AtomMenuModel(Delegate* delegate)
    : ui::SimpleMenuModel(delegate),
      delegate_(delegate) {
}

AtomMenuModel::~AtomMenuModel() {
}

void AtomMenuModel::MenuClosed() {
  ui::SimpleMenuModel::MenuClosed();
  FOR_EACH_OBSERVER(Observer, observers_, MenuClosed());
}

}  // namespace atom
