// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_ATOM_MENU_MODEL_H_
#define ATOM_BROWSER_UI_ATOM_MENU_MODEL_H_

#include <map>

#include "base/observer_list.h"
#include "ui/base/models/simple_menu_model.h"

namespace atom {

class AtomMenuModel : public ui::SimpleMenuModel {
 public:
  class Delegate : public ui::SimpleMenuModel::Delegate {
   public:
    virtual ~Delegate() {}
    virtual bool GetCommandAccelerator(int command_id,
                                       ui::Accelerator* accelerator,
                                       const std::string& context) = 0;

    bool GetAcceleratorForCommandId(int command_id,
                                    ui::Accelerator* accelerator) override;
  };

  class Observer {
   public:
    virtual ~Observer() {}

    // Notifies the menu has been closed.
    virtual void MenuClosed() {}
  };

  explicit AtomMenuModel(Delegate* delegate);
  virtual ~AtomMenuModel();

  void AddObserver(Observer* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(Observer* obs) { observers_.RemoveObserver(obs); }

  void SetRole(int index, const base::string16& role);
  base::string16 GetRoleAt(int index);

  // ui::SimpleMenuModel:
  void MenuClosed() override;

  Delegate* GetDelegate() { return delegate_; }

 private:
  Delegate* delegate_;  // weak ref.

  std::map<int, base::string16> roles_;  // command id -> role
  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(AtomMenuModel);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_ATOM_MENU_MODEL_H_
