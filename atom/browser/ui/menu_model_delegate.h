// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_MENU_MODEL_DELEGATE_H_
#define ATOM_BROWSER_UI_MENU_MODEL_DELEGATE_H_

#include "ui/base/models/simple_menu_model.h"

namespace atom {

class MenuModelDelegate : public ui::SimpleMenuModel::Delegate {
 public:
  virtual ~MenuModelDelegate() {}
  virtual bool GetCommandAccelerator(int command_id,
                                     ui::Accelerator* accelerator,
                                     const std::string& context) = 0;

  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) override;
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_MENU_MODEL_DELEGATE_H_
