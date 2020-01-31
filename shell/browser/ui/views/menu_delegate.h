// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_VIEWS_MENU_DELEGATE_H_
#define SHELL_BROWSER_UI_VIEWS_MENU_DELEGATE_H_

#include <memory>

#include "base/observer_list.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "ui/views/controls/menu/menu_delegate.h"

namespace views {
class MenuRunner;
class Button;
}  // namespace views

namespace electron {

class MenuBar;

class MenuDelegate : public views::MenuDelegate {
 public:
  explicit MenuDelegate(MenuBar* menu_bar);
  ~MenuDelegate() override;

  void RunMenu(ElectronMenuModel* model,
               views::Button* button,
               ui::MenuSourceType source_type);

  class Observer {
   public:
    virtual void OnBeforeExecuteCommand() = 0;
    virtual void OnMenuClosed() = 0;
  };

  void AddObserver(Observer* obs) { observers_.AddObserver(obs); }

  void RemoveObserver(const Observer* obs) { observers_.RemoveObserver(obs); }

 protected:
  // views::MenuDelegate:
  void ExecuteCommand(int id) override;
  void ExecuteCommand(int id, int mouse_event_flags) override;
  bool IsTriggerableEvent(views::MenuItemView* source,
                          const ui::Event& e) override;
  bool GetAccelerator(int id, ui::Accelerator* accelerator) const override;
  base::string16 GetLabel(int id) const override;
  void GetLabelStyle(int id, LabelStyle* style) const override;
  bool IsCommandEnabled(int id) const override;
  bool IsCommandVisible(int id) const override;
  bool IsItemChecked(int id) const override;
  void WillShowMenu(views::MenuItemView* menu) override;
  void WillHideMenu(views::MenuItemView* menu) override;
  void OnMenuClosed(views::MenuItemView* menu) override;
  views::MenuItemView* GetSiblingMenu(views::MenuItemView* menu,
                                      const gfx::Point& screen_point,
                                      views::MenuAnchorPosition* anchor,
                                      bool* has_mnemonics,
                                      views::MenuButton** button) override;

 private:
  MenuBar* menu_bar_;
  int id_;
  std::unique_ptr<views::MenuDelegate> adapter_;
  std::unique_ptr<views::MenuRunner> menu_runner_;

  // The menu button to switch to.
  views::MenuButton* button_to_open_ = nullptr;
  bool hold_first_switch_;

  base::ObserverList<Observer>::Unchecked observers_;

  DISALLOW_COPY_AND_ASSIGN(MenuDelegate);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_MENU_DELEGATE_H_
