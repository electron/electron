// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/atom_menu_model.h"

#include "base/stl_util.h"

namespace electron {

bool AtomMenuModel::Delegate::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  return GetAcceleratorForCommandIdWithParams(command_id, false, accelerator);
}

AtomMenuModel::AtomMenuModel(Delegate* delegate)
    : ui::SimpleMenuModel(delegate), delegate_(delegate) {}

AtomMenuModel::~AtomMenuModel() = default;

void AtomMenuModel::SetToolTip(int index, const base::string16& toolTip) {
  int command_id = GetCommandIdAt(index);
  toolTips_[command_id] = toolTip;
}

base::string16 AtomMenuModel::GetToolTipAt(int index) {
  const int command_id = GetCommandIdAt(index);
  const auto iter = toolTips_.find(command_id);
  return iter == std::end(toolTips_) ? base::string16() : iter->second;
}

void AtomMenuModel::SetRole(int index, const base::string16& role) {
  int command_id = GetCommandIdAt(index);
  roles_[command_id] = role;
}

base::string16 AtomMenuModel::GetRoleAt(int index) {
  const int command_id = GetCommandIdAt(index);
  const auto iter = roles_.find(command_id);
  return iter == std::end(roles_) ? base::string16() : iter->second;
}

void AtomMenuModel::SetSublabel(int index, const base::string16& sublabel) {
  int command_id = GetCommandIdAt(index);
  sublabels_[command_id] = sublabel;
}

base::string16 AtomMenuModel::GetSublabelAt(int index) const {
  int command_id = GetCommandIdAt(index);
  const auto iter = sublabels_.find(command_id);
  return iter == std::end(sublabels_) ? base::string16() : iter->second;
}

bool AtomMenuModel::GetAcceleratorAtWithParams(
    int index,
    bool use_default_accelerator,
    ui::Accelerator* accelerator) const {
  if (delegate_) {
    return delegate_->GetAcceleratorForCommandIdWithParams(
        GetCommandIdAt(index), use_default_accelerator, accelerator);
  }
  return false;
}

bool AtomMenuModel::ShouldRegisterAcceleratorAt(int index) const {
  if (delegate_) {
    return delegate_->ShouldRegisterAcceleratorForCommandId(
        GetCommandIdAt(index));
  }
  return true;
}

bool AtomMenuModel::WorksWhenHiddenAt(int index) const {
  if (delegate_) {
    return delegate_->ShouldCommandIdWorkWhenHidden(GetCommandIdAt(index));
  }
  return true;
}

void AtomMenuModel::MenuWillClose() {
  ui::SimpleMenuModel::MenuWillClose();
  for (Observer& observer : observers_) {
    observer.OnMenuWillClose();
  }
}

void AtomMenuModel::MenuWillShow() {
  ui::SimpleMenuModel::MenuWillShow();
  for (Observer& observer : observers_) {
    observer.OnMenuWillShow();
  }
}

AtomMenuModel* AtomMenuModel::GetSubmenuModelAt(int index) {
  return static_cast<AtomMenuModel*>(
      ui::SimpleMenuModel::GetSubmenuModelAt(index));
}

}  // namespace electron
