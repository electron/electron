// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_

#include "base/memory/raw_ptr.h"
#include "shell/browser/native_window_observer.h"
#include "shell/browser/ui/views/menu_delegate.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/accessible_pane_view.h"

namespace views {
class MenuButton;
}

namespace electron {

class ElectronMenuModel;
class NativeWindow;
class RootView;

class MenuBar : public views::AccessiblePaneView,
                private MenuDelegate::Observer,
                private NativeWindowObserver {
  METADATA_HEADER(MenuBar, views::AccessiblePaneView)

 public:
  MenuBar(NativeWindow* window, RootView* root_view);
  ~MenuBar() override;

  // disable copy
  MenuBar(const MenuBar&) = delete;
  MenuBar& operator=(const MenuBar&) = delete;

  // Replaces current menu with a new one.
  void SetMenu(ElectronMenuModel* menu_model);

  // Shows or hides menu-bar mnemonic underlines.
  void SetAcceleratorVisibility(bool visible);

  // Returns true if an enabled, visible menu-bar item has mnemonic |key|.
  bool HasAccelerator(char16_t key);

  // Opens the submenu or executes the command whose mnemonic is |key|.
  void ActivateAccelerator(char16_t key);

  // Returns the number of top-level items in the menu model.
  size_t GetItemCount() const;

  // Gets the submenu model and button at the specified screen point.
  bool GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                    ElectronMenuModel** menu_model,
                                    views::MenuButton** button);

  // Finds an adjacent submenu header or command, wrapping around and skipping
  // separators, hidden items, and disabled items. |next| selects the following
  // item when true and the preceding one when false. Returns false if there
  // is no eligible item other than |current_id|.
  bool GetSiblingMenuButtonByDirection(int current_id,
                                       bool next,
                                       views::MenuButton** button);

  // Selects |button| without executing commands. |open_submenu| opens submenu
  // headers immediately and preserves this mode when selecting commands.
  void SelectMenuButton(views::MenuButton* button, bool open_submenu);

  void ViewHierarchyChanged(
      const views::ViewHierarchyChangedDetails& details) override;

 private:
  // MenuDelegate::Observer:
  void OnBeforeExecuteCommand() override;
  void OnMenuClosed() override;

  // NativeWindowObserver:
  void OnWindowBlur() override;
  void OnWindowFocus() override;

  // views::AccessiblePaneView:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool SetPaneFocusAndFocusDefault() override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void OnThemeChanged() override;

  // views::FocusChangeListener:
  void OnDidChangeFocus(View* focused_before, View* focused_now) override;

  void ButtonPressed(size_t id, const ui::Event& event);

  void RebuildChildren();
  void UpdateViewColors();
  void RefreshColorCache(const ui::NativeTheme* theme);
  View* FindAccelChild(char16_t key);

  SkColor background_color_;
#if BUILDFLAG(IS_LINUX)
  SkColor enabled_color_;
  SkColor disabled_color_;
#endif

  raw_ptr<NativeWindow> window_;
  raw_ptr<RootView> root_view_;
  raw_ptr<ElectronMenuModel> menu_model_ = nullptr;
  bool accelerator_installed_ = false;
  bool open_menu_on_navigation_ = false;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_
