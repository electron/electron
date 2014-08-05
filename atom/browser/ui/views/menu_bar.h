// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_
#define ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_

#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/menu_button_listener.h"
#include "ui/views/view.h"

namespace ui {
class MenuModel;
}

namespace views {
class MenuButton;
}

namespace atom {

class MenuDelegate;

class MenuBar : public views::View,
                public views::ButtonListener,
                public views::MenuButtonListener {
 public:
  MenuBar();
  virtual ~MenuBar();

  // Replaces current menu with a new one.
  void SetMenu(ui::MenuModel* menu_model);

  // Returns there are how many items in the root menu.
  int GetItemCount() const;

  // Get the menu under specified screen point.
  bool GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                    ui::MenuModel** menu_model,
                                    views::MenuButton** button);

 protected:
  // views::View:
  virtual const char* GetClassName() const OVERRIDE;

  // views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

  // views::MenuButtonListener:
  virtual void OnMenuButtonClicked(views::View* source,
                                   const gfx::Point& point) OVERRIDE;

 private:
  SkColor background_color_;

#if defined(USE_X11)
  SkColor enabled_color_;
  SkColor disabled_color_;
  SkColor highlight_color_;
  SkColor hover_color_;
#endif

  ui::MenuModel* menu_model_;
  scoped_ptr<MenuDelegate> menu_delegate_;

  DISALLOW_COPY_AND_ASSIGN(MenuBar);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_
