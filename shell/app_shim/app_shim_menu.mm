// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app_shim/app_shim_menu.h"

#include <utility>

#include "ui/base/accelerators/accelerator.h"
#include "ui/base/models/image_model.h"

namespace electron {

class AppShimMenu::Item {
 public:
  Item(mojom::AppShimMenuItemPtr data,
       std::vector<uint32_t> path,
       AppShimMenu::Client* client)
      : data_(std::move(data)) {
    if (data_->submenu) {
      submenu_ = std::make_unique<AppShimMenu>(std::move(*data_->submenu),
                                               std::move(path), client);
      data_->submenu.reset();
    }
  }

  const mojom::AppShimMenuItem& data() const { return *data_; }
  AppShimMenu* submenu() const { return submenu_.get(); }

  // What a built NSMenuItem can't be given afterwards.
  bool HasShapeOf(const mojom::AppShimMenuItem& other) const {
    return data_->type == other.type && data_->command_id == other.command_id &&
           data_->role == other.role && data_->tool_tip == other.tool_tip &&
           data_->custom_type == other.custom_type &&
           data_->has_accelerator == other.has_accelerator &&
           data_->accelerator_key_code == other.accelerator_key_code &&
           data_->accelerator_modifiers == other.accelerator_modifiers &&
           data_->accelerator_shifted_char == other.accelerator_shifted_char &&
           data_->register_accelerator == other.register_accelerator &&
           data_->works_when_hidden == other.works_when_hidden &&
           data_->group_id == other.group_id &&
           !!submenu_ == other.submenu.has_value() &&
           (!submenu_ || submenu_->HasShapeOf(*other.submenu));
  }

  void Update(mojom::AppShimMenuItemPtr data) {
    if (submenu_)
      submenu_->Update(&*data->submenu);
    data_ = std::move(data);
    data_->submenu.reset();
  }

 private:
  mojom::AppShimMenuItemPtr data_;
  std::unique_ptr<AppShimMenu> submenu_;
};

AppShimMenu::AppShimMenu(std::vector<mojom::AppShimMenuItemPtr> items,
                         std::vector<uint32_t> path,
                         Client* client)
    : path_(std::move(path)),
      client_(client),
      model_(std::make_unique<ElectronMenuModel>(this)) {
  for (auto& item : items) {
    const size_t index = items_.size();
    std::vector<uint32_t> item_path = path_;
    item_path.push_back(index);
    items_.push_back(
        std::make_unique<Item>(std::move(item), std::move(item_path), client));
    const mojom::AppShimMenuItem& data = items_.back()->data();
    AppShimMenu* const submenu = items_.back()->submenu();
    // The model keeps the type of each item, and what it files under the
    // item's command id. The rest it asks this, its delegate, for.
    switch (static_cast<ui::MenuModel::ItemType>(data.type)) {
      case ui::MenuModel::TYPE_SEPARATOR:
        model_->InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);
        SyncModelItemAt(index);
        continue;
      case ui::MenuModel::TYPE_CHECK:
        model_->InsertCheckItemAt(index, data.command_id, data.label);
        break;
      case ui::MenuModel::TYPE_RADIO:
        model_->InsertRadioItemAt(index, data.command_id, data.label,
                                  data.group_id);
        break;
      case ui::MenuModel::TYPE_SUBMENU:
        if (submenu) {
          model_->InsertSubMenuAt(index, data.command_id, data.label,
                                  submenu->model());
          break;
        }
        [[fallthrough]];
      default:
        model_->InsertItemAt(index, data.command_id, data.label);
        break;
    }
    model_->SetToolTip(index, data.tool_tip);
    model_->SetRole(index, data.role);
    model_->SetCustomType(index, data.custom_type);
  }
  // Only the menu bar's model is told that a menu opens and closes.
  if (path_.empty())
    model_->AddObserver(this);
}

AppShimMenu::~AppShimMenu() {
  if (path_.empty())
    model_->RemoveObserver(this);
}

bool AppShimMenu::HasShapeOf(
    const std::vector<mojom::AppShimMenuItemPtr>& items) const {
  if (items.size() != items_.size())
    return false;
  for (size_t i = 0; i < items.size(); ++i) {
    if (!items_[i]->HasShapeOf(*items[i]))
      return false;
  }
  return true;
}

bool AppShimMenu::Update(std::vector<mojom::AppShimMenuItemPtr>* items) {
  if (!HasShapeOf(*items))
    return false;
  for (size_t i = 0; i < items->size(); ++i) {
    items_[i]->Update(std::move((*items)[i]));
    SyncModelItemAt(i);
  }
  return true;
}

const AppShimMenu::Item* AppShimMenu::ItemForCommandId(int command_id) const {
  const std::optional<size_t> index = model_->GetIndexOfCommandId(command_id);
  return index ? items_[*index].get() : nullptr;
}

void AppShimMenu::SyncModelItemAt(size_t index) {
  // A separator has no command id for the model to ask about.
  if (model_->GetTypeAt(index) == ui::MenuModel::TYPE_SEPARATOR)
    model_->SetVisibleAt(index, items_[index]->data().visible);
}

bool AppShimMenu::IsCommandIdChecked(int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item && item->data().checked;
}

bool AppShimMenu::IsCommandIdEnabled(int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item && item->data().enabled;
}

bool AppShimMenu::IsCommandIdVisible(int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item && item->data().visible;
}

std::u16string AppShimMenu::GetLabelForCommandId(int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item ? item->data().label : std::u16string();
}

std::u16string AppShimMenu::GetSecondaryLabelForCommandId(
    int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item ? item->data().secondary_label : std::u16string();
}

ui::ImageModel AppShimMenu::GetIconForCommandId(int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item && !item->data().icon.isNull()
             ? ui::ImageModel::FromImageSkia(item->data().icon)
             : ui::ImageModel();
}

void AppShimMenu::ExecuteCommand(int command_id, int event_flags) {
  const std::optional<size_t> index = model_->GetIndexOfCommandId(command_id);
  if (!index)
    return;
  std::vector<uint32_t> item_path = path_;
  item_path.push_back(*index);
  client_->OnMenuItemActivated(item_path, command_id, event_flags);
}

void AppShimMenu::OnMenuWillShow(ui::SimpleMenuModel* source) {
  client_->OnMenuWillShow();
}

bool AppShimMenu::GetAcceleratorForCommandIdWithParams(
    int command_id,
    bool use_default_accelerator,
    ui::Accelerator* accelerator) const {
  const Item* item = ItemForCommandId(command_id);
  if (!item || !item->data().has_accelerator)
    return false;
  const mojom::AppShimMenuItem& data = item->data();
  *accelerator =
      ui::Accelerator(static_cast<ui::KeyboardCode>(data.accelerator_key_code),
                      data.accelerator_modifiers);
  if (data.accelerator_shifted_char)
    accelerator->shifted_char = data.accelerator_shifted_char;
  return true;
}

bool AppShimMenu::ShouldRegisterAcceleratorForCommandId(int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item && item->data().register_accelerator;
}

bool AppShimMenu::ShouldCommandIdWorkWhenHidden(int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item && item->data().works_when_hidden;
}

std::u16string AppShimMenu::GetAccessibilityLabelForCommandId(
    int command_id) const {
  const Item* item = ItemForCommandId(command_id);
  return item ? item->data().accessibility_label : std::u16string();
}

bool AppShimMenu::GetSharingItemForCommandId(
    int command_id,
    ElectronMenuModel::SharingItem* item) const {
  return false;
}

void AppShimMenu::OnMenuWillClose() {
  client_->OnMenuWillClose();
}

}  // namespace electron
