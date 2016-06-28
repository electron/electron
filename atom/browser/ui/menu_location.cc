// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/menu_location.h"

#include "base/memory/ptr_util.h"

namespace atom {

MenuLocation::MenuLocation(std::string location, AtomMenuModel* model)
    : location_(location),
      model_(model) {
}

MenuLocation::~MenuLocation() {
}

void MenuLocation::AddObserver(atom::AtomMenuModel::Observer* obs) {
  model_->AddObserver(obs);
}

void MenuLocation::RemoveObserver(atom::AtomMenuModel::Observer* obs) {
  model_->RemoveObserver(obs);
}

base::string16 MenuLocation::GetRoleAt(int index) {
  return model_->GetRoleAt(index);
}

bool MenuLocation::HasIcons() const {
  return model_->HasIcons();
}

int MenuLocation::GetItemCount() const {
  return model_->GetItemCount();
}

ui::MenuModel::ItemType MenuLocation::GetTypeAt(int index) const {
  return model_->GetTypeAt(index);
}

ui::MenuSeparatorType MenuLocation::GetSeparatorTypeAt(int index) const {
  return model_->GetSeparatorTypeAt(index);
}

int MenuLocation::GetCommandIdAt(int index) const {
  return model_->GetCommandIdAt(index);
}

base::string16 MenuLocation::GetLabelAt(int index) const {
  return model_->GetLabelAt(index);
}

base::string16 MenuLocation::GetSublabelAt(int index) const {
  return model_->GetSublabelAt(index);
}

base::string16 MenuLocation::GetMinorTextAt(int index) const {
  return model_->GetMinorTextAt(index);
}

bool MenuLocation::IsItemDynamicAt(int index) const {
  return model_->IsItemDynamicAt(index);
}

bool MenuLocation::GetAcceleratorAt(int index,
                                        ui::Accelerator* accelerator) const {
  AtomMenuModel::Delegate* delegate = model_->GetDelegate();
  if (delegate) {
    return delegate->GetCommandAccelerator(GetCommandIdAt(index),
                                           accelerator,
                                           location_);
  }
  return false;
}

bool MenuLocation::IsItemCheckedAt(int index) const {
  return model_->IsItemCheckedAt(index);
}

int MenuLocation::GetGroupIdAt(int index) const {
  return model_->GetGroupIdAt(index);
}

bool MenuLocation::GetIconAt(int index, gfx::Image* icon) {
  return model_->GetIconAt(index, icon);
}

ui::ButtonMenuItemModel* MenuLocation::GetButtonMenuItemAt(
    int index) const {
  return model_->GetButtonMenuItemAt(index);
}

bool MenuLocation::IsEnabledAt(int index) const {
  return model_->IsEnabledAt(index);
}

bool MenuLocation::IsVisibleAt(int index) const {
  return model_->IsVisibleAt(index);
}

ui::MenuModel* MenuLocation::GetSubmenuModelAt(int index) const {
  if (ContainsKey(submenu_models_, index)) {
    return submenu_models_.get(index);
  } else {
    auto model = static_cast<atom::AtomMenuModel*>(
        model_->GetSubmenuModelAt(index));
    auto model_context = new MenuLocation(location_, model);
    submenu_models_.set(index, base::WrapUnique(model_context));
    return model_context;
  }
}

void MenuLocation::HighlightChangedTo(int index) {
  model_->HighlightChangedTo(index);
}

void MenuLocation::ActivatedAt(int index) {
  ActivatedAt(index, 0);
}

void MenuLocation::ActivatedAt(int index, int event_flags) {
  AtomMenuModel::Delegate* delegate = model_->GetDelegate();
  if (delegate)
    delegate->RunCommand(GetCommandIdAt(index), event_flags, location_);
}

void MenuLocation::MenuWillShow() {
  model_->MenuWillShow();
}

void MenuLocation::MenuClosed() {
  model_->MenuClosed();
}

void MenuLocation::SetMenuModelDelegate(ui::MenuModelDelegate* delegate) {
  model_->SetMenuModelDelegate(delegate);
}

ui::MenuModelDelegate* MenuLocation::GetMenuModelDelegate() const {
  return model_->GetMenuModelDelegate();
}

}  // namespace atom
