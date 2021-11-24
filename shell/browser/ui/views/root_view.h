// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_ROOT_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_ROOT_VIEW_H_

#include <memory>

#include "shell/browser/ui/accelerator_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/view.h"
#include "ui/views/view_tracker.h"

namespace content {
struct NativeWebKeyboardEvent;
}

namespace electron {

class ElectronMenuModel;
class MenuBar;
class NativeWindow;

class RootView : public views::View {
 public:
  explicit RootView(NativeWindow* window);
  ~RootView() override;

  // disable copy
  RootView(const RootView&) = delete;
  RootView& operator=(const RootView&) = delete;

  void SetMenu(ElectronMenuModel* menu_model);
  bool HasMenu() const;
  int GetMenuBarHeight() const;
  void SetAutoHideMenuBar(bool auto_hide);
  bool IsMenuBarAutoHide() const;
  void SetMenuBarVisibility(bool visible);
  bool IsMenuBarVisible() const;
  void HandleKeyEvent(const content::NativeWebKeyboardEvent& event);
  void ResetAltState();
  void RestoreFocus();
  // Register/Unregister accelerators supported by the menu model.
  void RegisterAcceleratorsWithFocusManager(ElectronMenuModel* menu_model);
  void UnregisterAcceleratorsWithFocusManager();

  // views::View:
  void Layout() override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

 private:
  // Parent window, weak ref.
  NativeWindow* window_;

  // Menu bar.
  std::unique_ptr<MenuBar> menu_bar_;
  bool menu_bar_autohide_ = false;
  bool menu_bar_visible_ = false;
  bool menu_bar_alt_pressed_ = false;

  // Map from accelerator to menu item's command id.
  accelerator_util::AcceleratorTable accelerator_table_;

  std::unique_ptr<views::ViewTracker> last_focused_view_tracker_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_ROOT_VIEW_H_
