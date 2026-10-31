// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app_shim/app_shim_menu.h"

#include <utility>

#include "ui/base/accelerators/accelerator.h"
#include "ui/base/models/image_model.h"

namespace electron {

class AppShimMenu::Item : public ElectronMenuModel::Item {
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

  // ElectronMenuModel::Item:
  ui::MenuModel::ItemType GetType() const override {
    return static_cast<ui::MenuModel::ItemType>(data_->type);
  }
  int GetCommandId() const override { return data_->command_id; }
  std::u16string GetLabel() const override { return data_->label; }
  std::u16string GetSecondaryLabel() const override {
    return data_->secondary_label;
  }
  std::u16string GetToolTip() const override { return data_->tool_tip; }
  std::u16string GetAccessibilityLabel() const override {
    return data_->accessibility_label;
  }
  std::u16string GetRole() const override { return data_->role; }
  std::u16string GetCustomType() const override { return data_->custom_type; }
  ui::ImageModel GetIcon() const override {
    return data_->icon.isNull() ? ui::ImageModel()
                                : ui::ImageModel::FromImageSkia(data_->icon);
  }
  bool GetAccelerator(ui::Accelerator* accelerator) const override {
    if (!data_->has_accelerator)
      return false;
    *accelerator = ui::Accelerator(
        static_cast<ui::KeyboardCode>(data_->accelerator_key_code),
        data_->accelerator_modifiers);
    if (data_->accelerator_shifted_char)
      accelerator->shifted_char = data_->accelerator_shifted_char;
    return true;
  }
  bool ShouldRegisterAccelerator() const override {
    return data_->register_accelerator;
  }
  bool WorksWhenHidden() const override { return data_->works_when_hidden; }
  bool IsChecked() const override { return data_->checked; }
  bool IsEnabled() const override { return data_->enabled; }
  bool IsVisible() const override { return data_->visible; }
  int GetGroupId() const override { return data_->group_id; }
  ElectronMenuModel* GetSubmenuModel() const override {
    return submenu_ ? submenu_->model() : nullptr;
  }
  std::optional<ElectronMenuModel::SharingItem> GetSharingItem() override {
    return std::nullopt;
  }
  const ElectronMenuModel::Badge* GetBadge() const override { return nullptr; }

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
    std::vector<uint32_t> item_path = path_;
    item_path.push_back(items_.size());
    items_.push_back(
        std::make_unique<Item>(std::move(item), std::move(item_path), client));
    model_->InsertItemAt(items_.size() - 1, items_.back().get());
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
  for (size_t i = 0; i < items->size(); ++i)
    items_[i]->Update(std::move((*items)[i]));
  return true;
}

void AppShimMenu::ActivatedAt(size_t index, int event_flags) {
  std::vector<uint32_t> item_path = path_;
  item_path.push_back(index);
  client_->OnMenuItemActivated(item_path, items_[index]->data().command_id,
                               event_flags);
}

void AppShimMenu::MenuWillShow() {
  client_->OnMenuWillShow();
}

void AppShimMenu::OnMenuWillClose() {
  client_->OnMenuWillClose();
}

}  // namespace electron
