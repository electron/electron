// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_
#define ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_

#include <memory>

#include "atom/browser/ui/atom_menu_model.h"
#include "atom/browser/ui/views/menu_delegate.h"
#include "atom/browser/ui/views/root_view.h"
#include "ui/views/accessible_pane_view.h"
#include "ui/views/controls/button/menu_button_listener.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"

namespace views {
class MenuButton;
}

namespace atom {

class MenuBarColorUpdater : public views::FocusChangeListener {
 public:
  explicit MenuBarColorUpdater(MenuBar* menu_bar);
  ~MenuBarColorUpdater() override;

  void OnDidChangeFocus(views::View* focused_before,
                        views::View* focused_now) override;
  void OnWillChangeFocus(views::View* focused_before,
                         views::View* focused_now) override {}

 private:
  MenuBar* menu_bar_;
};

class MenuBar : public views::AccessiblePaneView,
                public views::MenuButtonListener,
                public atom::MenuDelegate::Observer {
 public:
  static const char kViewClassName[];

  explicit MenuBar(RootView* window);
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

  // atom::MenuDelegate::Observer:
  void OnBeforeExecuteCommand() override;
  void OnMenuClosed() override;

  // views::AccessiblePaneView:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool SetPaneFocus(views::View* initial_focus) override;
  void RemovePaneFocus() override;

 protected:
  // views::View:
  const char* GetClassName() const override;

  // views::MenuButtonListener:
  void OnMenuButtonClicked(views::MenuButton* source,
                           const gfx::Point& point,
                           const ui::Event* event) override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;

 private:
  friend class MenuBarColorUpdater;

  void RebuildChildren();
  void UpdateViewColors();

  void RefreshColorCache(const ui::NativeTheme* theme = nullptr);
  SkColor background_color_;
#if defined(USE_X11)
  SkColor enabled_color_;
  SkColor disabled_color_;
#endif

  RootView* window_ = nullptr;
  AtomMenuModel* menu_model_ = nullptr;

  View* FindAccelChild(base::char16 key);

  bool has_focus_ = true;

  std::unique_ptr<MenuBarColorUpdater> color_updater_;

  DISALLOW_COPY_AND_ASSIGN(MenuBar);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_MENU_BAR_H_
