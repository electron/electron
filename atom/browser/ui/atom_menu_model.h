// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <map>

#include "base/observer_list.h"
#include "ui/base/models/simple_menu_model.h"

namespace atom {

class AtomMenuModel : public ui::SimpleMenuModel {
 public:
  class Delegate : public ui::SimpleMenuModel::Delegate {
   public:
    virtual ~Delegate() {}

    virtual bool GetAcceleratorForCommandIdWithParams(
        int command_id,
        bool use_default_accelerator,
        ui::Accelerator* accelerator) const = 0;

   private:
    // ui::SimpleMenuModel::Delegate:
    bool GetAcceleratorForCommandId(int command_id,
                                    ui::Accelerator* accelerator) {
      return GetAcceleratorForCommandIdWithParams(
          command_id, false, accelerator);
    }
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
  bool GetAcceleratorAtWithParams(int index,
                                  bool use_default_accelerator,
                                  ui::Accelerator* accelerator) const;

  // ui::SimpleMenuModel:
  void MenuClosed() override;

  using SimpleMenuModel::GetSubmenuModelAt;
  AtomMenuModel* GetSubmenuModelAt(int index);

 private:
  Delegate* delegate_;  // weak ref.

  std::map<int, base::string16> roles_;  // command id -> role
  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(AtomMenuModel);
};

}  // namespace atom
