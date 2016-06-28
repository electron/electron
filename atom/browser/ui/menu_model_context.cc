// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/menu_model_context.h"

#include "base/memory/ptr_util.h"

namespace atom {

MenuModelContext::MenuModelContext(std::string name, AtomMenuModel* model)
    : name_(name),
      model_(model) {
}

MenuModelContext::~MenuModelContext() {
}

void MenuModelContext::AddObserver(atom::AtomMenuModel::Observer* obs) {
  model_->AddObserver(obs);
}

void MenuModelContext::RemoveObserver(atom::AtomMenuModel::Observer* obs) {
  model_->RemoveObserver(obs);
}

base::string16 MenuModelContext::GetRoleAt(int index) {
  return model_->GetRoleAt(index);
}

bool MenuModelContext::HasIcons() const {
  return model_->HasIcons();
}

int MenuModelContext::GetItemCount() const {
  return model_->GetItemCount();
}

ui::MenuModel::ItemType MenuModelContext::GetTypeAt(int index) const {
  return model_->GetTypeAt(index);
}

ui::MenuSeparatorType MenuModelContext::GetSeparatorTypeAt(int index) const {
  return model_->GetSeparatorTypeAt(index);
}

int MenuModelContext::GetCommandIdAt(int index) const {
  return model_->GetCommandIdAt(index);
}

base::string16 MenuModelContext::GetLabelAt(int index) const {
  return model_->GetLabelAt(index);
}

base::string16 MenuModelContext::GetSublabelAt(int index) const {
  return model_->GetSublabelAt(index);
}

base::string16 MenuModelContext::GetMinorTextAt(int index) const {
  return model_->GetMinorTextAt(index);
}

bool MenuModelContext::IsItemDynamicAt(int index) const {
  return model_->IsItemDynamicAt(index);
}

bool MenuModelContext::GetAcceleratorAt(int index,
                                        ui::Accelerator* accelerator) const {
  AtomMenuModel::Delegate* delegate = model_->GetDelegate();
  if (delegate) {
    return delegate->GetCommandAccelerator(GetCommandIdAt(index),
                                           accelerator,
                                           name_);
  }
  return false;
}

bool MenuModelContext::IsItemCheckedAt(int index) const {
  return model_->IsItemCheckedAt(index);
}

int MenuModelContext::GetGroupIdAt(int index) const {
  return model_->GetGroupIdAt(index);
}

bool MenuModelContext::GetIconAt(int index, gfx::Image* icon) {
  return model_->GetIconAt(index, icon);
}

ui::ButtonMenuItemModel* MenuModelContext::GetButtonMenuItemAt(
    int index) const {
  return model_->GetButtonMenuItemAt(index);
}

bool MenuModelContext::IsEnabledAt(int index) const {
  return model_->IsEnabledAt(index);
}

bool MenuModelContext::IsVisibleAt(int index) const {
  return model_->IsVisibleAt(index);
}

ui::MenuModel* MenuModelContext::GetSubmenuModelAt(int index) const {
  if (ContainsKey(submenu_models_, index)) {
    return submenu_models_.get(index);
  } else {
    auto model = static_cast<atom::AtomMenuModel*>(
        model_->GetSubmenuModelAt(index));
    auto model_context = new MenuModelContext(name_, model);
    submenu_models_.set(index, base::WrapUnique(model_context));
    return model_context;
  }
}

void MenuModelContext::HighlightChangedTo(int index) {
  model_->HighlightChangedTo(index);
}

void MenuModelContext::ActivatedAt(int index) {
  ActivatedAt(index, 0);
}

void MenuModelContext::ActivatedAt(int index, int event_flags) {
  AtomMenuModel::Delegate* delegate = model_->GetDelegate();
  if (delegate)
    delegate->RunCommand(GetCommandIdAt(index), event_flags, name_);
}

void MenuModelContext::MenuWillShow() {
  model_->MenuWillShow();
}

void MenuModelContext::MenuClosed() {
  model_->MenuClosed();
}

void MenuModelContext::SetMenuModelDelegate(ui::MenuModelDelegate* delegate) {
  model_->SetMenuModelDelegate(delegate);
}

ui::MenuModelDelegate* MenuModelContext::GetMenuModelDelegate() const {
  return model_->GetMenuModelDelegate();
}

}  // namespace atom
