// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_MENU_DELEGATE_H_
#define ATOM_BROWSER_UI_VIEWS_MENU_DELEGATE_H_

#include <vector>

#include "ui/views/controls/menu/menu_delegate.h"

namespace views {
class MenuModelAdapter;
class MenuRunner;
}

namespace ui {
class MenuModel;
}

namespace atom {

class MenuBar;

class MenuDelegate : public views::MenuDelegate {
 public:
  MenuDelegate(MenuBar* menu_bar);
  virtual ~MenuDelegate();

  void RunMenu(ui::MenuModel* model, views::MenuButton* button);

 protected:
  // views::MenuDelegate:
  virtual void ExecuteCommand(int id) OVERRIDE;
  virtual void ExecuteCommand(int id, int mouse_event_flags) OVERRIDE;
  virtual bool IsTriggerableEvent(views::MenuItemView* source,
                                  const ui::Event& e) OVERRIDE;
  virtual bool GetAccelerator(int id,
                              ui::Accelerator* accelerator) OVERRIDE;
  virtual base::string16 GetLabel(int id) const OVERRIDE;
  virtual const gfx::FontList* GetLabelFontList(int id) const OVERRIDE;
  virtual bool IsCommandEnabled(int id) const OVERRIDE;
  virtual bool IsItemChecked(int id) const OVERRIDE;
  virtual void SelectionChanged(views::MenuItemView* menu) OVERRIDE;
  virtual void WillShowMenu(views::MenuItemView* menu) OVERRIDE;
  virtual void WillHideMenu(views::MenuItemView* menu) OVERRIDE;
  virtual views::MenuItemView* GetSiblingMenu(
      views::MenuItemView* menu,
      const gfx::Point& screen_point,
      views::MenuItemView::AnchorPosition* anchor,
      bool* has_mnemonics,
      views::MenuButton** button);

 private:
  // Gets the cached menu item view from the model.
  views::MenuItemView* BuildMenu(ui::MenuModel* model);

  // Returns delegate for current item.
  views::MenuDelegate* delegate() const { return delegates_[id_]; }

  MenuBar* menu_bar_;
  scoped_ptr<views::MenuRunner> menu_runner_;

  // Current item's id.
  int id_;
  // Cached menu items, managed by MenuRunner.
  std::vector<views::MenuItemView*> items_;
  // Cached menu delegates for each menu item, managed by us.
  std::vector<views::MenuDelegate*> delegates_;

  DISALLOW_COPY_AND_ASSIGN(MenuDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_MENU_DELEGATE_H_
