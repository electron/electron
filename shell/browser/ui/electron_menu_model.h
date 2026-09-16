// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_ELECTRON_MENU_MODEL_H_
#define ELECTRON_SHELL_BROWSER_UI_ELECTRON_MENU_MODEL_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "build/build_config.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/models/image_model.h"
#include "ui/base/models/menu_model.h"
#include "url/gurl.h"

namespace electron {

// A ui::MenuModel over Items that it does not own.
class ElectronMenuModel : public ui::MenuModel {
 public:
#if BUILDFLAG(IS_MAC)
  struct SharingItem {
    SharingItem();
    SharingItem(SharingItem&&);
    SharingItem(const SharingItem&);
    SharingItem& operator=(const SharingItem&);
    SharingItem& operator=(SharingItem&&);
    ~SharingItem();

    std::optional<std::vector<std::string>> texts;
    std::optional<std::vector<GURL>> urls;
    std::optional<std::vector<base::FilePath>> file_paths;
  };

  struct Badge {
    Badge();
    Badge(Badge&&);
    Badge(const Badge&);
    Badge& operator=(const Badge&);
    Badge& operator=(Badge&&);
    ~Badge();

    std::string type;  // "alerts", "updates", "new-items" or "none"
    std::optional<int> count;
    std::optional<std::string> content;
  };
#endif

  class Item {
   public:
    virtual ~Item() = default;
    virtual ItemType GetType() const = 0;
    virtual int GetCommandId() const = 0;
    virtual std::u16string GetLabel() const = 0;
    virtual std::u16string GetSecondaryLabel() const = 0;
    virtual std::u16string GetToolTip() const = 0;
    virtual std::u16string GetAccessibilityLabel() const = 0;
    virtual std::u16string GetRole() const = 0;
    virtual std::u16string GetCustomType() const = 0;
    virtual ui::ImageModel GetIcon() const = 0;
    virtual bool GetAccelerator(ui::Accelerator* accelerator) const = 0;
    virtual bool ShouldRegisterAccelerator() const = 0;
    virtual bool WorksWhenHidden() const = 0;
    virtual bool IsChecked() const = 0;
    virtual bool IsEnabled() const = 0;
    virtual bool IsVisible() const = 0;
    virtual int GetGroupId() const = 0;
    virtual ElectronMenuModel* GetSubmenuModel() const = 0;
#if BUILDFLAG(IS_MAC)
    virtual std::optional<SharingItem> GetSharingItem() = 0;
    virtual const Badge* GetBadge() const = 0;
#endif
  };

  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void ActivatedAt(size_t index, int event_flags) = 0;
    virtual void MenuWillShow() = 0;
  };

  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override = default;

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

  // |item| must outlive the model.
  void InsertItemAt(size_t index, Item* item);
  Item* item_at(size_t index) const { return items_[index].item; }
  std::optional<size_t> GetIndexOfCommandId(int command_id) const;
  std::u16string GetToolTipAt(size_t index) const;
  std::u16string GetCustomTypeAt(size_t index) const;
  std::u16string GetAccessibilityLabelAt(size_t index) const;
  std::u16string GetRoleAt(size_t index) const;
  bool GetAcceleratorAtWithParams(size_t index,
                                  bool use_default_accelerator,
                                  ui::Accelerator* accelerator) const;
  bool ShouldRegisterAcceleratorAt(size_t index) const;
  bool WorksWhenHiddenAt(size_t index) const;
#if BUILDFLAG(IS_MAC)
  bool GetSharingItemAt(size_t index, SharingItem* item) const;
  // The SharingItem of this menu as a whole.
  void SetSharingItem(SharingItem item);
  [[nodiscard]] const std::optional<SharingItem>& sharing_item() const {
    return sharing_item_;
  }
  bool GetBadgeAt(size_t index, Badge* badge) const;
#endif

  // ui::MenuModel:
  base::WeakPtr<ui::MenuModel> AsWeakPtr() override;
  size_t GetItemCount() const override;
  ItemType GetTypeAt(size_t index) const override;
  ui::MenuSeparatorType GetSeparatorTypeAt(size_t index) const override;
  int GetCommandIdAt(size_t index) const override;
  std::u16string GetLabelAt(size_t index) const override;
  std::u16string GetSecondaryLabelAt(size_t index) const override;
  bool IsItemDynamicAt(size_t index) const override;
  bool GetAcceleratorAt(size_t index,
                        ui::Accelerator* accelerator) const override;
  bool IsItemCheckedAt(size_t index) const override;
  int GetGroupIdAt(size_t index) const override;
  ui::ImageModel GetIconAt(size_t index) const override;
  ui::ButtonMenuItemModel* GetButtonMenuItemAt(size_t index) const override;
  bool IsEnabledAt(size_t index) const override;
  bool IsVisibleAt(size_t index) const override;
  ui::MenuModel* GetSubmenuModelAt(size_t index) const override;
  void ActivatedAt(size_t index) override;
  void ActivatedAt(size_t index, int event_flags) override;
  void MenuWillShow() override;
  void MenuWillClose() override;

  ElectronMenuModel* GetSubmenuModelAt(size_t index);

  base::WeakPtr<ElectronMenuModel> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  struct ItemRef {
    raw_ptr<Item> item;
    // Copied here because views looks items up by scanning these.
    ItemType type;
    int command_id;
  };
  const Item& ItemAt(size_t index) const { return *items_[index].item; }

  raw_ptr<Delegate> delegate_;
  std::vector<ItemRef> items_;
#if BUILDFLAG(IS_MAC)
  std::optional<SharingItem> sharing_item_;
#endif
  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<ElectronMenuModel> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_ELECTRON_MENU_MODEL_H_
