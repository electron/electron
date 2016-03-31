// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_UI_ELECTRON_MENU_MODEL_H_
#define ELECTRON_BROWSER_UI_ELECTRON_MENU_MODEL_H_

#include <map>

#include "base/observer_list.h"
#include "ui/base/models/simple_menu_model.h"

namespace electron {

class ElectronMenuModel : public ui::SimpleMenuModel {
 public:
  class Delegate : public ui::SimpleMenuModel::Delegate {
   public:
    virtual ~Delegate() {}
  };

  class Observer {
   public:
    virtual ~Observer() {}

    // Notifies the menu has been closed.
    virtual void MenuClosed() {}
  };

  explicit ElectronMenuModel(Delegate* delegate);
  virtual ~ElectronMenuModel();

  void AddObserver(Observer* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(Observer* obs) { observers_.RemoveObserver(obs); }

  void SetRole(int index, const base::string16& role);
  base::string16 GetRoleAt(int index);

  // ui::SimpleMenuModel:
  void MenuClosed() override;

 private:
  Delegate* delegate_;  // weak ref.

  std::map<int, base::string16> roles_;
  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(ElectronMenuModel);
};

}  // namespace electron

#endif  // ELECTRON_BROWSER_UI_ELECTRON_MENU_MODEL_H_
