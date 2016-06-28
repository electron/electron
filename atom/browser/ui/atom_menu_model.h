// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_ATOM_MENU_MODEL_H_
#define ATOM_BROWSER_UI_ATOM_MENU_MODEL_H_

#include <map>

#include "atom/browser/ui/menu_model_delegate.h"
#include "base/observer_list.h"

namespace atom {

class AtomMenuModel : public ui::SimpleMenuModel {
 public:
  class Observer {
   public:
    virtual ~Observer() {}

    // Notifies the menu has been closed.
    virtual void MenuClosed() {}
  };

  explicit AtomMenuModel(MenuModelDelegate* delegate);
  virtual ~AtomMenuModel();

  void AddObserver(Observer* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(Observer* obs) { observers_.RemoveObserver(obs); }

  void SetRole(int index, const base::string16& role);
  base::string16 GetRoleAt(int index);

  ui::MenuModel* GetTrayModel();

  // ui::SimpleMenuModel:
  void MenuClosed() override;

  MenuModelDelegate* GetDelegate() { return delegate_; }

 private:
  MenuModelDelegate* delegate_;  // weak ref.

  std::unique_ptr<ui::MenuModel> tray_model_;
  std::map<int, base::string16> roles_;  // command id -> role
  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(AtomMenuModel);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_ATOM_MENU_MODEL_H_
