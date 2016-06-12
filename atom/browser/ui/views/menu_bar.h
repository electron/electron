// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

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

  // Shows underline under accelerators.
  void SetAcceleratorVisibility(bool visible);

  // Returns which submenu has accelerator |key|, -1 would be returned when
  // there is no matching submenu.
  int GetAcceleratorIndex(base::char16 key);

  // Shows the submenu whose accelerator is |key|.
  void ActivateAccelerator(base::char16 key);

  // Returns there are how many items in the root menu.
  int GetItemCount() const;

  // Get the menu under specified screen point.
  bool GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                    ui::MenuModel** menu_model,
                                    views::MenuButton** button);

 protected:
  // views::View:
  const char* GetClassName() const override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // views::MenuButtonListener:
  void OnMenuButtonClicked(views::MenuButton* source,
                           const gfx::Point& point,
                           const ui::Event* event) override;


 private:
  SkColor background_color_;

#if defined(USE_X11)
  SkColor enabled_color_;
  SkColor disabled_color_;
  SkColor highlight_color_;
  SkColor hover_color_;
#endif

  ui::MenuModel* menu_model_;

  DISALLOW_COPY_AND_ASSIGN(MenuBar);
};

}  // namespace atom
