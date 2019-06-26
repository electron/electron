// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_VIEWS_ROOT_VIEW_H_
#define SHELL_BROWSER_UI_VIEWS_ROOT_VIEW_H_

#include <memory>

#include "shell/browser/native_window_observer.h"
#include "shell/browser/ui/accelerator_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/view.h"
#include "ui/views/view_tracker.h"

namespace content {
struct NativeWebKeyboardEvent;
}

namespace electron {

class AtomMenuModel;
class MenuBar;
class TitleBar;
class NativeWindow;

class RootView : public views::View, public NativeWindowObserver {
 public:
  explicit RootView(NativeWindow* window);
  ~RootView() override;

  void SetMenu(AtomMenuModel* menu_model);
  bool HasMenu() const;
  int GetMenuBarHeight() const;
  void SetAutoHideMenuBar(bool auto_hide);
  bool IsMenuBarAutoHide() const;
  void SetMenuBarVisibility(bool visible);
  bool IsMenuBarVisible() const;
  void SetTitleBarVisibility(bool visible);
  bool IsTitleBarVisible() const;
  void HandleKeyEvent(const content::NativeWebKeyboardEvent& event);
  void ResetAltState();
  void RestoreFocus();
  // Register/Unregister accelerators supported by the menu model.
  void RegisterAcceleratorsWithFocusManager(AtomMenuModel* menu_model);
  void UnregisterAcceleratorsWithFocusManager();
  void SetInsets(const gfx::Insets& insets);
  gfx::Insets insets() const { return insets_; }
  int NonClientHitTest(const gfx::Point& point);
  void UpdateWindowIcon();
  void UpdateWindowTitle();

  // views::View:
  void Layout() override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

  // NativeWindowObserver:
  void OnWindowEnterFullScreen() override;
  void OnWindowLeaveFullScreen() override;

  TitleBar* custom_title_bar() { return title_bar_.get(); }
  NativeWindow* window() { return window_; }

 private:
  // Parent window, weak ref.
  NativeWindow* window_;

  // Title bar.
  std::unique_ptr<TitleBar> title_bar_;
  bool title_bar_visible_ = true;

  // Menu bar.
  std::unique_ptr<MenuBar> menu_bar_;
  bool menu_bar_autohide_ = false;
  bool menu_bar_visible_ = false;
  bool menu_bar_alt_pressed_ = false;

  gfx::Insets insets_;

  // Map from accelerator to menu item's command id.
  accelerator_util::AcceleratorTable accelerator_table_;

  std::unique_ptr<views::ViewTracker> last_focused_view_tracker_;

  DISALLOW_COPY_AND_ASSIGN(RootView);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_ROOT_VIEW_H_
