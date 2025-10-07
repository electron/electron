// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/accelerator_util.h"

#include <stdio.h>

#include <string>
#include <string_view>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "shell/common/keyboard_util.h"

namespace accelerator_util {

bool StringToAccelerator(const std::string_view shortcut,
                         ui::Accelerator* accelerator) {
  if (!base::IsStringASCII(shortcut)) {
    LOG(ERROR) << "The accelerator string can only contain ASCII characters, "
                  "invalid string: "
               << "\"" << shortcut << "\"";

    return false;
  }

  const std::vector<std::string_view> tokens = base::SplitStringPiece(
      shortcut, "+", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // Now, parse it into an accelerator.
  int modifiers = ui::EF_NONE;
  ui::KeyboardCode key = ui::VKEY_UNKNOWN;
  std::optional<char16_t> shifted_char;
  for (const std::string_view token : tokens) {
    ui::KeyboardCode code = electron::KeyboardCodeFromStr(token, &shifted_char);
    if (shifted_char)
      modifiers |= ui::EF_SHIFT_DOWN;
    switch (code) {
      // The token can be a modifier.
      case ui::VKEY_SHIFT:
        modifiers |= ui::EF_SHIFT_DOWN;
        break;
      case ui::VKEY_CONTROL:
        modifiers |= ui::EF_CONTROL_DOWN;
        break;
      case ui::VKEY_MENU:
        modifiers |= ui::EF_ALT_DOWN;
        break;
      case ui::VKEY_COMMAND:
        modifiers |= ui::EF_COMMAND_DOWN;
        break;
      case ui::VKEY_ALTGR:
        modifiers |= ui::EF_ALTGR_DOWN;
        break;
      // Or it is a normal key.
      default:
        key = code;
    }
  }

  if (key == ui::VKEY_UNKNOWN) {
    LOG(WARNING) << shortcut << " doesn't contain a valid key";
    return false;
  }

  *accelerator = ui::Accelerator(key, modifiers);
  accelerator->shifted_char = shifted_char;
  return true;
}

void GenerateAcceleratorTable(AcceleratorTable* table,
                              electron::ElectronMenuModel* model) {
  size_t count = model->GetItemCount();
  for (size_t i = 0; i < count; ++i) {
    electron::ElectronMenuModel::ItemType type = model->GetTypeAt(i);
    if (type == electron::ElectronMenuModel::TYPE_SUBMENU) {
      auto* submodel = model->GetSubmenuModelAt(i);
      GenerateAcceleratorTable(table, submodel);
    } else {
      ui::Accelerator accelerator;
      if (model->ShouldRegisterAcceleratorAt(i)) {
        if (model->GetAcceleratorAtWithParams(i, true, &accelerator)) {
          MenuItem item = {i, model};
          (*table)[accelerator] = item;
        }
      }
    }
  }
}

bool TriggerAcceleratorTableCommand(AcceleratorTable* table,
                                    const ui::Accelerator& accelerator) {
  const auto iter = table->find(accelerator);
  if (iter != std::end(*table)) {
    const accelerator_util::MenuItem& item = iter->second;
    if (item.model->IsEnabledAt(item.position)) {
      const auto event_flags =
          accelerator.MaskOutKeyEventFlags(accelerator.modifiers());
      item.model->ActivatedAt(item.position, event_flags);
      return true;
    }
  }
  return false;
}

}  // namespace accelerator_util
