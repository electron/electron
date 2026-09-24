// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/electron_menu_model.h"

#include <utility>

#include "base/check_op.h"
#include "ui/base/models/menu_model_delegate.h"

namespace electron {

#if BUILDFLAG(IS_MAC)
ElectronMenuModel::SharingItem::SharingItem() = default;
ElectronMenuModel::SharingItem::SharingItem(SharingItem&&) = default;
ElectronMenuModel::SharingItem::SharingItem(const SharingItem&) = default;
ElectronMenuModel::SharingItem& ElectronMenuModel::SharingItem::operator=(
    const SharingItem&) = default;
ElectronMenuModel::SharingItem& ElectronMenuModel::SharingItem::operator=(
    SharingItem&&) = default;
ElectronMenuModel::SharingItem::~SharingItem() = default;

ElectronMenuModel::Badge::Badge() = default;
ElectronMenuModel::Badge::Badge(Badge&&) = default;
ElectronMenuModel::Badge::Badge(const Badge&) = default;
ElectronMenuModel::Badge& ElectronMenuModel::Badge::operator=(const Badge&) =
    default;
ElectronMenuModel::Badge& ElectronMenuModel::Badge::operator=(Badge&&) =
    default;
ElectronMenuModel::Badge::~Badge() = default;
#endif

ElectronMenuModel::ElectronMenuModel(Delegate* delegate)
    : delegate_(delegate) {}

ElectronMenuModel::~ElectronMenuModel() = default;

void ElectronMenuModel::InsertItemAt(size_t index, Item* item) {
  CHECK_LE(index, items_.size());
  items_.insert(items_.begin() + index,
                ItemRef{item, item->GetType(), item->GetCommandId()});
}

std::optional<size_t> ElectronMenuModel::GetIndexOfCommandId(
    int command_id) const {
  for (size_t i = 0; i < items_.size(); ++i) {
    if (items_[i].command_id == command_id)
      return i;
  }
  return std::nullopt;
}

std::u16string ElectronMenuModel::GetToolTipAt(size_t index) const {
  return ItemAt(index).GetToolTip();
}

std::u16string ElectronMenuModel::GetCustomTypeAt(size_t index) const {
  return ItemAt(index).GetCustomType();
}

std::u16string ElectronMenuModel::GetAccessibilityLabelAt(size_t index) const {
  return ItemAt(index).GetAccessibilityLabel();
}

std::u16string ElectronMenuModel::GetRoleAt(size_t index) const {
  return ItemAt(index).GetRole();
}

bool ElectronMenuModel::GetAcceleratorAtWithParams(
    size_t index,
    bool use_default_accelerator,
    ui::Accelerator* accelerator) const {
  return ItemAt(index).GetAccelerator(accelerator);
}

bool ElectronMenuModel::ShouldRegisterAcceleratorAt(size_t index) const {
  return ItemAt(index).ShouldRegisterAccelerator();
}

bool ElectronMenuModel::WorksWhenHiddenAt(size_t index) const {
  return ItemAt(index).WorksWhenHidden();
}

#if BUILDFLAG(IS_MAC)
bool ElectronMenuModel::GetSharingItemAt(size_t index,
                                         SharingItem* item) const {
  std::optional<SharingItem> sharing_item =
      items_[index].item->GetSharingItem();
  if (!sharing_item)
    return false;
  *item = std::move(*sharing_item);
  return true;
}

void ElectronMenuModel::SetSharingItem(SharingItem item) {
  sharing_item_.emplace(std::move(item));
}

bool ElectronMenuModel::GetBadgeAt(size_t index, Badge* badge) const {
  const Badge* item_badge = ItemAt(index).GetBadge();
  if (!item_badge)
    return false;
  *badge = *item_badge;
  return true;
}
#endif

base::WeakPtr<ui::MenuModel> ElectronMenuModel::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

size_t ElectronMenuModel::GetItemCount() const {
  return items_.size();
}

ui::MenuModel::ItemType ElectronMenuModel::GetTypeAt(size_t index) const {
  return items_[index].type;
}

ui::MenuSeparatorType ElectronMenuModel::GetSeparatorTypeAt(
    size_t index) const {
  return ui::NORMAL_SEPARATOR;
}

int ElectronMenuModel::GetCommandIdAt(size_t index) const {
  return items_[index].command_id;
}

std::u16string ElectronMenuModel::GetLabelAt(size_t index) const {
  return ItemAt(index).GetLabel();
}

std::u16string ElectronMenuModel::GetSecondaryLabelAt(size_t index) const {
  return ItemAt(index).GetSecondaryLabel();
}

bool ElectronMenuModel::IsItemDynamicAt(size_t index) const {
  return false;
}

bool ElectronMenuModel::GetAcceleratorAt(size_t index,
                                         ui::Accelerator* accelerator) const {
  return ItemAt(index).GetAccelerator(accelerator);
}

bool ElectronMenuModel::IsItemCheckedAt(size_t index) const {
  const ItemType type = items_[index].type;
  return (type == TYPE_CHECK || type == TYPE_RADIO) &&
         ItemAt(index).IsChecked();
}

int ElectronMenuModel::GetGroupIdAt(size_t index) const {
  return ItemAt(index).GetGroupId();
}

ui::ImageModel ElectronMenuModel::GetIconAt(size_t index) const {
  return ItemAt(index).GetIcon();
}

ui::ButtonMenuItemModel* ElectronMenuModel::GetButtonMenuItemAt(
    size_t index) const {
  return nullptr;
}

bool ElectronMenuModel::IsEnabledAt(size_t index) const {
  return GetTypeAt(index) != TYPE_SEPARATOR && ItemAt(index).IsEnabled();
}

bool ElectronMenuModel::IsVisibleAt(size_t index) const {
  return ItemAt(index).IsVisible();
}

ui::MenuModel* ElectronMenuModel::GetSubmenuModelAt(size_t index) const {
  return ItemAt(index).GetSubmenuModel();
}

ElectronMenuModel* ElectronMenuModel::GetSubmenuModelAt(size_t index) {
  return ItemAt(index).GetSubmenuModel();
}

void ElectronMenuModel::ActivatedAt(size_t index) {
  ActivatedAt(index, 0);
}

void ElectronMenuModel::ActivatedAt(size_t index, int event_flags) {
  delegate_->ActivatedAt(index, event_flags);
}

void ElectronMenuModel::MenuWillShow() {
  delegate_->MenuWillShow();
  observers_.Notify(&Observer::OnMenuWillShow);
}

void ElectronMenuModel::MenuWillClose() {
  observers_.Notify(&Observer::OnMenuWillClose);
}

}  // namespace electron
