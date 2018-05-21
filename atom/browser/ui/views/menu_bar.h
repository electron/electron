// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_
#define ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_

#include "atom/browser/ui/atom_menu_model.h"
#include "ui/views/controls/button/menu_button_listener.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"

namespace views {
class MenuButton;
}

namespace atom {

class MenuDelegate;

class MenuBar : public views::View,
                public views::MenuButtonListener,
                public views::FocusChangeListener {
 public:
  static const char kViewClassName[];

  explicit MenuBar(views::View* window);
  ~MenuBar() override;

  // Replaces current menu with a new one.
  void SetMenu(AtomMenuModel* menu_model);

  // Shows underline under accelerators.
  void SetAcceleratorVisibility(bool visible);

  // Returns true if the submenu has accelerator |key|
  bool HasAccelerator(base::char16 key);

  // Shows the submenu whose accelerator is |key|.
  void ActivateAccelerator(base::char16 key);

  // Returns there are how many items in the root menu.
  int GetItemCount() const;

  // Get the menu under specified screen point.
  bool GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                    AtomMenuModel** menu_model,
                                    views::MenuButton** button);

 protected:
  // views::View:
  const char* GetClassName() const override;

  // views::MenuButtonListener:
  void OnMenuButtonClicked(views::MenuButton* source,
                           const gfx::Point& point,
                           const ui::Event* event) override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;

  // views::FocusChangeListener:
  void OnDidChangeFocus(View* focused_before, View* focused_now) override;
  void OnWillChangeFocus(View* focused_before, View* focused_now) override {}

 private:
  void RebuildChildren();
  void UpdateViewColors();

  void RefreshColorCache(const ui::NativeTheme* theme = nullptr);
  SkColor background_color_;
#if defined(USE_X11)
  SkColor enabled_color_;
  SkColor disabled_color_;
#endif

  views::View* window_ = nullptr;
  AtomMenuModel* menu_model_ = nullptr;

  View* FindAccelChild(base::char16 key);

  bool has_focus_ = true;

  DISALLOW_COPY_AND_ASSIGN(MenuBar);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_
