// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_SHIM_APP_SHIM_MENU_H_
#define ELECTRON_SHELL_APP_SHIM_APP_SHIM_MENU_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "electron/shell/common/app_shim.mojom.h"
#include "shell/browser/ui/electron_menu_model.h"

namespace electron {

// A menu, and the menus under it, that the host app described. It gives
// ElectronMenuController a model to build an NSMenu from, as it would have in
// the host's process, and passes on what the person does with it.
class AppShimMenu : public ElectronMenuModel::Delegate,
                    private ElectronMenuModel::Observer {
 public:
  class Client {
   public:
    virtual void OnMenuItemActivated(const std::vector<uint32_t>& path,
                                     int command_id,
                                     int event_flags) = 0;
    virtual void OnMenuWillShow() = 0;
    virtual void OnMenuWillClose() = 0;

   protected:
    virtual ~Client() = default;
  };

  // `client` must outlive this. `path` is where this menu is under the menu
  // bar: empty for the menu bar itself.
  AppShimMenu(std::vector<mojom::AppShimMenuItemPtr> items,
              std::vector<uint32_t> path,
              Client* client);
  ~AppShimMenu() override;

  AppShimMenu(const AppShimMenu&) = delete;
  AppShimMenu& operator=(const AppShimMenu&) = delete;

  ElectronMenuModel* model() { return model_.get(); }

  // Takes the state of `items` (labels, checks and so on) if they are the
  // same menu as this one, item for item. False, changing nothing, when they
  // are not, and an NSMenu built from this would be the wrong one.
  bool Update(std::vector<mojom::AppShimMenuItemPtr>* items);

 private:
  class Item;

  bool HasShapeOf(const std::vector<mojom::AppShimMenuItemPtr>& items) const;
  // Null when no item of this menu has `command_id`.
  const Item* ItemForCommandId(int command_id) const;
  // Gives the model the state of the item at `index` that it keeps for
  // itself and does not ask its delegate for.
  void SyncModelItemAt(size_t index);

  // ElectronMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  std::u16string GetLabelForCommandId(int command_id) const override;
  std::u16string GetSecondaryLabelForCommandId(int command_id) const override;
  ui::ImageModel GetIconForCommandId(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;
  void OnMenuWillShow(ui::SimpleMenuModel* source) override;
  bool GetAcceleratorForCommandIdWithParams(
      int command_id,
      bool use_default_accelerator,
      ui::Accelerator* accelerator) const override;
  bool ShouldRegisterAcceleratorForCommandId(int command_id) const override;
  bool ShouldCommandIdWorkWhenHidden(int command_id) const override;
  std::u16string GetAccessibilityLabelForCommandId(
      int command_id) const override;
  bool GetSharingItemForCommandId(
      int command_id,
      ElectronMenuModel::SharingItem* item) const override;

  // ElectronMenuModel::Observer:
  void OnMenuWillShow() override {}
  void OnMenuWillClose() override;

  const std::vector<uint32_t> path_;
  const raw_ptr<Client> client_;
  std::vector<std::unique_ptr<Item>> items_;
  std::unique_ptr<ElectronMenuModel> model_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_SHIM_APP_SHIM_MENU_H_
