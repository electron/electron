// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_model_adapter.h"

namespace atom {

MenuModelAdapter::MenuModelAdapter(AtomMenuModel* menu_model)
  : views::MenuModelAdapter(menu_model),
    menu_model_(menu_model) {
}

MenuModelAdapter::~MenuModelAdapter() {
}

bool MenuModelAdapter::GetAccelerator(int id,
                                      ui::Accelerator* accelerator) const {
  ui::MenuModel* model = menu_model_;
  int index = 0;
  if (ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index)) {
    return static_cast<AtomMenuModel*>(model)->
      GetAcceleratorAtWithParams(index, true, accelerator);
  }
  return false;
}

}  // namespace atom
