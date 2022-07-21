// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_ACCELERATOR_UTIL_H_
#define ELECTRON_SHELL_BROWSER_UI_ACCELERATOR_UTIL_H_

#include <map>
#include <string>

#include "shell/browser/ui/electron_menu_model.h"
#include "ui/base/accelerators/accelerator.h"

namespace accelerator_util {

typedef struct {
  size_t position;
  electron::ElectronMenuModel* model;
} MenuItem;
typedef std::map<ui::Accelerator, MenuItem> AcceleratorTable;

// Parse a string as an accelerator.
bool StringToAccelerator(const std::string& shortcut,
                         ui::Accelerator* accelerator);

// Generate a table that contains menu model's accelerators and command ids.
void GenerateAcceleratorTable(AcceleratorTable* table,
                              electron::ElectronMenuModel* model);

// Trigger command from the accelerators table.
bool TriggerAcceleratorTableCommand(AcceleratorTable* table,
                                    const ui::Accelerator& accelerator);

}  // namespace accelerator_util

#endif  // ELECTRON_SHELL_BROWSER_UI_ACCELERATOR_UTIL_H_
