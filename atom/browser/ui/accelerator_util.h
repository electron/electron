// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <map>
#include <string>

#include "ui/base/accelerators/accelerator.h"

namespace ui {
class MenuModel;
}

namespace accelerator_util {

typedef struct { int position; ui::MenuModel* model; } MenuItem;
typedef std::map<ui::Accelerator, MenuItem> AcceleratorTable;

// Parse a string as an accelerator.
bool StringToAccelerator(const std::string& description,
                         ui::Accelerator* accelerator);

// Set platform accelerator for the Accelerator.
void SetPlatformAccelerator(ui::Accelerator* accelerator);

// Generate a table that contains memu model's accelerators and command ids.
void GenerateAcceleratorTable(AcceleratorTable* table, ui::MenuModel* model);

// Trigger command from the accelerators table.
bool TriggerAcceleratorTableCommand(AcceleratorTable* table,
                                    const ui::Accelerator& accelerator);

}  // namespace accelerator_util
