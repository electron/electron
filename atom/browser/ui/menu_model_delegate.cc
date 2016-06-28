// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/menu_model_delegate.h"

namespace atom {

bool MenuModelDelegate::GetAcceleratorForCommandId(
    int command_id, ui::Accelerator* accelerator) {
  return GetCommandAccelerator(command_id, accelerator, "");
}

}  // namespace atom
