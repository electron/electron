// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_ELECTRON_MENU_MODEL_H_
#define ELECTRON_SHELL_BROWSER_UI_ELECTRON_MENU_MODEL_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/models/simple_menu_model.h"
#include "url/gurl.h"

namespace electron {

class ElectronMenuModel : public ui::SimpleMenuModel {
 public:
#if defined(OS_MAC)
  struct SharingItem {
    SharingItem();
    SharingItem(SharingItem&&);
    SharingItem(const SharingItem&) = delete;
    ~SharingItem();

    absl::optional<std::vector<std::string>> texts;
    absl::optional<std::vector<GURL>> urls;
    absl::optional<std::vector<base::FilePath>> file_paths;
  };
#endif

  class Delegate : public ui::SimpleMenuModel::Delegate {
   public:
    ~Delegate() override {}

    virtual bool GetAcceleratorForCommandIdWithParams(
        int command_id,
        bool use_default_accelerator,
        ui::Accelerator* accelerator) const = 0;

    virtual bool ShouldRegisterAcceleratorForCommandId(
        int command_id) const = 0;

    virtual bool ShouldCommandIdWorkWhenHidden(int command_id) const = 0;

#if defined(OS_MAC)
    virtual bool GetSharingItemForCommandId(int command_id,
                                            SharingItem* item) const = 0;
#endif

   private:
    // ui::SimpleMenuModel::Delegate:
    bool GetAcceleratorForCommandId(
        int command_id,
        ui::Accelerator* accelerator) const override;
  };

  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    // Notifies the menu will open.
    virtual void OnMenuWillShow() {}

    // Notifies the menu has been closed.
    virtual void OnMenuWillClose() {}
  };

  explicit ElectronMenuModel(Delegate* delegate);
  ~ElectronMenuModel() override;

  // disable copy
  ElectronMenuModel(const ElectronMenuModel&) = delete;
  ElectronMenuModel& operator=(const ElectronMenuModel&) = delete;

  void AddObserver(Observer* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(Observer* obs) { observers_.RemoveObserver(obs); }

  void SetToolTip(int index, const std::u16string& toolTip);
  std::u16string GetToolTipAt(int index);
  void SetRole(int index, const std::u16string& role);
  std::u16string GetRoleAt(int index);
  void SetSecondaryLabel(int index, const std::u16string& sublabel);
  std::u16string GetSecondaryLabelAt(int index) const override;
  bool GetAcceleratorAtWithParams(int index,
                                  bool use_default_accelerator,
                                  ui::Accelerator* accelerator) const;
  bool ShouldRegisterAcceleratorAt(int index) const;
  bool WorksWhenHiddenAt(int index) const;
#if defined(OS_MAC)
  // Return the SharingItem of menu item.
  bool GetSharingItemAt(int index, SharingItem* item) const;
  // Set/Get the SharingItem of this menu.
  void SetSharingItem(SharingItem item);
  const absl::optional<SharingItem>& GetSharingItem() const;
#endif

  // ui::SimpleMenuModel:
  void MenuWillClose() override;
  void MenuWillShow() override;

  base::WeakPtr<ElectronMenuModel> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  using SimpleMenuModel::GetSubmenuModelAt;
  ElectronMenuModel* GetSubmenuModelAt(int index);

 private:
  Delegate* delegate_;  // weak ref.

#if defined(OS_MAC)
  absl::optional<SharingItem> sharing_item_;
#endif

  std::map<int, std::u16string> toolTips_;   // command id -> tooltip
  std::map<int, std::u16string> roles_;      // command id -> role
  std::map<int, std::u16string> sublabels_;  // command id -> sublabel
  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<ElectronMenuModel> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_ELECTRON_MENU_MODEL_H_
