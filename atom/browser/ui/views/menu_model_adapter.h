// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_MENU_MODEL_ADAPTER_H_
#define ATOM_BROWSER_UI_VIEWS_MENU_MODEL_ADAPTER_H_

#include "atom/browser/ui/atom_menu_model.h"
#include "ui/views/controls/menu/menu_model_adapter.h"

namespace atom {

class MenuModelAdapter : public views::MenuModelAdapter {
 public:
  explicit MenuModelAdapter(AtomMenuModel* menu_model);
  virtual ~MenuModelAdapter();

 protected:
  bool GetAccelerator(int id, ui::Accelerator* accelerator) const override;

 private:
  AtomMenuModel* menu_model_;

  DISALLOW_COPY_AND_ASSIGN(MenuModelAdapter);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_MENU_MODEL_ADAPTER_H_
