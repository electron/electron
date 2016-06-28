// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_MENU_MODEL_CONTEXT_H_
#define ATOM_BROWSER_UI_MENU_MODEL_CONTEXT_H_

#include <string>

#include "atom/browser/ui/atom_menu_model.h"
#include "base/containers/scoped_ptr_hash_map.h"
#include "ui/base/models/menu_model.h"

namespace atom {

class MenuModelContext : public ui::MenuModel {
 public:
  explicit MenuModelContext(std::string name, atom::AtomMenuModel* model);
  virtual ~MenuModelContext();

  // Defer to atom::AtomMenuModel
  void AddObserver(atom::AtomMenuModel::Observer* obs);
  void RemoveObserver(atom::AtomMenuModel::Observer* obs);
  base::string16 GetRoleAt(int index);

  // ui::MenuModel:
  bool HasIcons() const override;
  int GetItemCount() const override;
  ItemType GetTypeAt(int index) const override;
  ui::MenuSeparatorType GetSeparatorTypeAt(int index) const override;
  int GetCommandIdAt(int index) const override;
  base::string16 GetLabelAt(int index) const override;
  base::string16 GetSublabelAt(int index) const override;
  base::string16 GetMinorTextAt(int index) const override;
  bool IsItemDynamicAt(int index) const override;
  bool GetAcceleratorAt(int index, ui::Accelerator* accelerator) const override;
  bool IsItemCheckedAt(int index) const override;
  int GetGroupIdAt(int index) const override;
  bool GetIconAt(int index, gfx::Image* icon) override;
  ui::ButtonMenuItemModel* GetButtonMenuItemAt(int index) const override;
  bool IsEnabledAt(int index) const override;
  bool IsVisibleAt(int index) const override;
  void HighlightChangedTo(int index) override;
  void ActivatedAt(int index) override;
  void ActivatedAt(int index, int event_flags) override;
  ui::MenuModel* GetSubmenuModelAt(int index) const override;
  void MenuWillShow() override;
  void MenuClosed() override;
  void SetMenuModelDelegate(
      ui::MenuModelDelegate* menu_model_delegate) override;
  ui::MenuModelDelegate* GetMenuModelDelegate() const override;

 private:
  std::string name_;
  AtomMenuModel* model_;
  mutable base::ScopedPtrHashMap<int, std::unique_ptr<ui::MenuModel>> submenu_models_;

  DISALLOW_COPY_AND_ASSIGN(MenuModelContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_ATOM_MENU_MODEL_H_
