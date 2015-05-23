// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_VIEWS_H_
#define ATOM_BROWSER_NATIVE_WINDOW_VIEWS_H_

#include "atom/browser/native_window.h"

#include <string>
#include <vector>

#include "atom/browser/ui/accelerator_util.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class UnhandledKeyboardEventHandler;
}

namespace atom {

class GlobalMenuBarX11;
class MenuBar;
class WindowStateWatcher;

class NativeWindowViews : public NativeWindow,
                          public views::WidgetDelegateView,
                          public views::WidgetObserver {
 public:
  explicit NativeWindowViews(content::WebContents* web_contents,
                            const mate::Dictionary& options);
  virtual ~NativeWindowViews();

  // NativeWindow:
  void Close() override;
  void CloseImmediately() override;
  void Focus(bool focus) override;
  bool IsFocused() override;
  void Show() override;
  void ShowInactive() override;
  void Hide() override;
  bool IsVisible() override;
  void Maximize() override;
  void Unmaximize() override;
  bool IsMaximized() override;
  void Minimize() override;
  void Restore() override;
  bool IsMinimized() override;
  void SetFullScreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  void SetContentSize(const gfx::Size& size) override;
  gfx::Size GetContentSize() override;
  void SetMinimumSize(const gfx::Size& size) override;
  gfx::Size GetMinimumSize() override;
  void SetMaximumSize(const gfx::Size& size) override;
  gfx::Size GetMaximumSize() override;
  void SetResizable(bool resizable) override;
  bool IsResizable() override;
  void SetAlwaysOnTop(bool top) override;
  bool IsAlwaysOnTop() override;
  void Center() override;
  void SetTitle(const std::string& title) override;
  std::string GetTitle() override;
  void FlashFrame(bool flash) override;
  void SetSkipTaskbar(bool skip) override;
  void SetKiosk(bool kiosk) override;
  bool IsKiosk() override;
  void SetMenu(ui::MenuModel* menu_model) override;
  gfx::NativeWindow GetNativeWindow() override;
  void SetOverlayIcon(const gfx::Image& overlay,
                      const std::string& description) override;
  void SetProgressBar(double value) override;
  void SetAutoHideMenuBar(bool auto_hide) override;
  bool IsMenuBarAutoHide() override;
  void SetMenuBarVisibility(bool visible) override;
  bool IsMenuBarVisible() override;
  void SetVisibleOnAllWorkspaces(bool visible) override;
  bool IsVisibleOnAllWorkspaces() override;

  gfx::AcceleratedWidget GetAcceleratedWidget();

  SkRegion* draggable_region() const { return draggable_region_.get(); }
  views::Widget* widget() const { return window_.get(); }

 private:
  // NativeWindow:
  void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) override;

  // views::WidgetObserver:
  void OnWidgetActivationChanged(
      views::Widget* widget, bool active) override;

  // views::WidgetDelegate:
  void DeleteDelegate() override;
  views::View* GetInitiallyFocusedView() override;
  bool CanResize() const override;
  bool CanMaximize() const override;
  bool CanMinimize() const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldHandleSystemCommands() const override;
  gfx::ImageSkia GetWindowAppIcon() override;
  gfx::ImageSkia GetWindowIcon() override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  views::View* GetContentsView() override;
  bool ShouldDescendIntoChildForEventHandling(
     gfx::NativeView child,
     const gfx::Point& location) override;
  views::ClientView* CreateClientView(views::Widget* widget) override;
  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override;
#if defined(OS_WIN)
  bool ExecuteWindowsCommand(int command_id) override;
#endif

  // brightray::InspectableWebContentsDelegate:
  gfx::ImageSkia GetDevToolsWindowIcon() override;
#if defined(USE_X11)
  void GetDevToolsWindowWMClass(
      std::string* name, std::string* class_name) override;
#endif

  // content::WebContentsDelegate:
  void ActivateContents(content::WebContents* contents) override;
  void HandleKeyboardEvent(
      content::WebContents*,
      const content::NativeWebKeyboardEvent& event) override;

  // views::View:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

  // Register accelerators supported by the menu model.
  void RegisterAccelerators(ui::MenuModel* menu_model);

  // Converts between client area and window area, since we include the menu bar
  // in client area we need to substract/add menu bar's height in convertions.
  gfx::Rect ContentBoundsToWindowBounds(const gfx::Rect& content_bounds);

  // Returns the restore state for the window.
  ui::WindowShowState GetRestoredState();

  scoped_ptr<views::Widget> window_;
  views::View* web_view_;  // Managed by inspectable_web_contents_.

  scoped_ptr<MenuBar> menu_bar_;
  bool menu_bar_autohide_;
  bool menu_bar_visible_;
  bool menu_bar_alt_pressed_;

#if defined(USE_X11)
  scoped_ptr<GlobalMenuBarX11> global_menu_bar_;

  // Handles window state events.
  scoped_ptr<WindowStateWatcher> window_state_watcher_;
#elif defined(OS_WIN)
  // Records window was whether restored from minimized state or maximized
  // state.
  bool is_minimized_;
#endif

  // Handles unhandled keyboard messages coming back from the renderer process.
  scoped_ptr<views::UnhandledKeyboardEventHandler> keyboard_event_handler_;

  // Map from accelerator to menu item's command id.
  accelerator_util::AcceleratorTable accelerator_table_;

  bool use_content_size_;
  bool resizable_;
  std::string title_;
  gfx::Size minimum_size_;
  gfx::Size maximum_size_;

  scoped_ptr<SkRegion> draggable_region_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowViews);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_VIEWS_H_
