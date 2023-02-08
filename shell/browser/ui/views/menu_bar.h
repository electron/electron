// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_

#include "shell/browser/native_window_observer.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/ui/views/menu_delegate.h"
#include "shell/browser/ui/views/root_view.h"
#include "ui/views/accessible_pane_view.h"

namespace views {
class MenuButton;
}

namespace electron {

class MenuBar : public views::AccessiblePaneView,
                public MenuDelegate::Observer,
                public NativeWindowObserver {
 public:
  static const char kViewClassName[];

  MenuBar(NativeWindow* window, RootView* root_view);
  ~MenuBar() override;

  // disable copy
  MenuBar(const MenuBar&) = delete;
  MenuBar& operator=(const MenuBar&) = delete;

  // Replaces current menu with a new one.
  void SetMenu(ElectronMenuModel* menu_model);

  // Shows underline under accelerators.
  void SetAcceleratorVisibility(bool visible);

  // Returns true if the submenu has accelerator |key|
  bool HasAccelerator(char16_t key);

  // Shows the submenu whose accelerator is |key|.
  void ActivateAccelerator(char16_t key);

  // Returns there are how many items in the root menu.
  size_t GetItemCount() const;

  // Get the menu under specified screen point.
  bool GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                    ElectronMenuModel** menu_model,
                                    views::MenuButton** button);

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
  void OnThemeChanged() override;

  // views::FocusChangeListener:
  void OnDidChangeFocus(View* focused_before, View* focused_now) override;

  // views::View:
  const char* GetClassName() const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

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

  NativeWindow* window_;
  RootView* root_view_;
  ElectronMenuModel* menu_model_ = nullptr;
  bool accelerator_installed_ = false;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_
