// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NATIVE_WINDOW_MAC_H_
#define SHELL_BROWSER_NATIVE_WINDOW_MAC_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/native_window.h"
#include "ui/views/controls/native/native_view_host.h"

@class AtomNSWindow;
@class AtomNSWindowDelegate;
@class AtomPreviewItem;
@class AtomTouchBar;
@class CustomWindowButtonView;

namespace electron {

class RootViewMac;

class NativeWindowMac : public NativeWindow {
 public:
  NativeWindowMac(const gin_helper::Dictionary& options, NativeWindow* parent);
  ~NativeWindowMac() override;

  // NativeWindow:
  void SetContentView(views::View* view) override;
  void Close() override;
  void CloseImmediately() override;
  void Focus(bool focus) override;
  bool IsFocused() override;
  void Show() override;
  void ShowInactive() override;
  void Hide() override;
  bool IsVisible() override;
  bool IsEnabled() override;
  void SetEnabled(bool enable) override;
  void Maximize() override;
  void Unmaximize() override;
  bool IsMaximized() override;
  void Minimize() override;
  void Restore() override;
  bool IsMinimized() override;
  void SetFullScreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetBounds(const gfx::Rect& bounds, bool animate = false) override;
  gfx::Rect GetBounds() override;
  bool IsNormal() override;
  gfx::Rect GetNormalBounds() override;
  void SetContentSizeConstraints(
      const extensions::SizeConstraints& size_constraints) override;
  void SetResizable(bool resizable) override;
  bool MoveAbove(const std::string& sourceId) override;
  void MoveTop() override;
  bool IsResizable() override;
  void SetMovable(bool movable) override;
  void SetAspectRatio(double aspect_ratio,
                      const gfx::Size& extra_size) override;
  void PreviewFile(const std::string& path,
                   const std::string& display_name) override;
  void CloseFilePreview() override;
  bool IsMovable() override;
  void SetMinimizable(bool minimizable) override;
  bool IsMinimizable() override;
  void SetMaximizable(bool maximizable) override;
  bool IsMaximizable() override;
  void SetFullScreenable(bool fullscreenable) override;
  bool IsFullScreenable() override;
  void SetClosable(bool closable) override;
  bool IsClosable() override;
  void SetAlwaysOnTop(ui::ZOrderLevel z_order,
                      const std::string& level,
                      int relativeLevel) override;
  ui::ZOrderLevel GetZOrderLevel() override;
  void Center() override;
  void Invalidate() override;
  void SetTitle(const std::string& title) override;
  std::string GetTitle() override;
  void FlashFrame(bool flash) override;
  void SetSkipTaskbar(bool skip) override;
  void SetExcludedFromShownWindowsMenu(bool excluded) override;
  bool IsExcludedFromShownWindowsMenu() override;
  void SetSimpleFullScreen(bool simple_fullscreen) override;
  bool IsSimpleFullScreen() override;
  void SetKiosk(bool kiosk) override;
  bool IsKiosk() override;
  void SetBackgroundColor(SkColor color) override;
  SkColor GetBackgroundColor() override;
  void SetHasShadow(bool has_shadow) override;
  bool HasShadow() override;
  void SetOpacity(const double opacity) override;
  double GetOpacity() override;
  void SetRepresentedFilename(const std::string& filename) override;
  std::string GetRepresentedFilename() override;
  void SetDocumentEdited(bool edited) override;
  bool IsDocumentEdited() override;
  void SetIgnoreMouseEvents(bool ignore, bool forward) override;
  void SetContentProtection(bool enable) override;
  void SetFocusable(bool focusable) override;
  void AddBrowserView(NativeBrowserView* browser_view) override;
  void RemoveBrowserView(NativeBrowserView* browser_view) override;
  void SetParentWindow(NativeWindow* parent) override;
  content::DesktopMediaID GetDesktopMediaID() const override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  gfx::AcceleratedWidget GetAcceleratedWidget() const override;
  NativeWindowHandle GetNativeWindowHandle() const override;
  void SetProgressBar(double progress, const ProgressState state) override;
  void SetOverlayIcon(const gfx::Image& overlay,
                      const std::string& description) override;

  void SetVisibleOnAllWorkspaces(bool visible) override;
  bool IsVisibleOnAllWorkspaces() override;

  void SetAutoHideCursor(bool auto_hide) override;

  void SelectPreviousTab() override;
  void SelectNextTab() override;
  void MergeAllWindows() override;
  void MoveTabToNewWindow() override;
  void ToggleTabBar() override;
  bool AddTabbedWindow(NativeWindow* window) override;

  bool SetWindowButtonVisibility(bool visible) override;

  void SetVibrancy(const std::string& type) override;
  void SetTouchBar(
      std::vector<gin_helper::PersistentDictionary> items) override;
  void RefreshTouchBarItem(const std::string& item_id) override;
  void SetEscapeTouchBarItem(gin_helper::PersistentDictionary item) override;
  void SetGTKDarkThemeEnabled(bool use_dark_theme) override {}

  gfx::Rect ContentBoundsToWindowBounds(const gfx::Rect& bounds) const override;
  gfx::Rect WindowBoundsToContentBounds(const gfx::Rect& bounds) const override;

  // Use a custom content view instead of Chromium's BridgedContentView.
  void OverrideNSWindowContentView();

  // Set the attribute of NSWindow while work around a bug of zoom button.
  void SetStyleMask(bool on, NSUInteger flag);
  void SetCollectionBehavior(bool on, NSUInteger flag);
  void SetWindowLevel(int level);

  // Custom traffic light positioning
  void RepositionTrafficLights();
  void SetExitingFullScreen(bool flag);

  enum class TitleBarStyle {
    NORMAL,
    HIDDEN,
    HIDDEN_INSET,
    CUSTOM_BUTTONS_ON_HOVER,
  };
  TitleBarStyle title_bar_style() const { return title_bar_style_; }

  AtomPreviewItem* preview_item() const { return preview_item_.get(); }
  AtomTouchBar* touch_bar() const { return touch_bar_.get(); }
  bool zoom_to_page_width() const { return zoom_to_page_width_; }
  bool fullscreen_window_title() const { return fullscreen_window_title_; }
  bool always_simple_fullscreen() const { return always_simple_fullscreen_; }
  bool exiting_fullscreen() const { return exiting_fullscreen_; }

 protected:
  // views::WidgetDelegate:
  bool CanResize() const override;
  views::View* GetContentsView() override;

 private:
  // Add custom layers to the content view.
  void AddContentViewLayers(bool minimizable, bool closable);

  void InternalSetParentWindow(NativeWindow* parent, bool attach);
  void SetForwardMouseMessages(bool forward);

  AtomNSWindow* window_;  // Weak ref, managed by widget_.

  base::scoped_nsobject<AtomNSWindowDelegate> window_delegate_;
  base::scoped_nsobject<AtomPreviewItem> preview_item_;
  base::scoped_nsobject<AtomTouchBar> touch_bar_;
  base::scoped_nsobject<CustomWindowButtonView> buttons_view_;

  // Event monitor for scroll wheel event.
  id wheel_event_monitor_;

  // The NSView that used as contentView of window.
  //
  // For frameless window it would fill the whole window.
  base::scoped_nsobject<NSView> container_view_;

  // The views::View that fills the client area.
  std::unique_ptr<RootViewMac> root_view_;

  bool is_kiosk_ = false;
  bool was_fullscreen_ = false;
  bool zoom_to_page_width_ = false;
  bool fullscreen_window_title_ = false;
  bool resizable_ = true;
  bool exiting_fullscreen_ = false;
  gfx::Point traffic_light_position_;

  NSInteger attention_request_id_ = 0;  // identifier from requestUserAttention

  // The presentation options before entering kiosk mode.
  NSApplicationPresentationOptions kiosk_options_;

  // The "titleBarStyle" option.
  TitleBarStyle title_bar_style_ = TitleBarStyle::NORMAL;

  // The visibility mode of window button controls when explicitly set through
  // setWindowButtonVisibility().
  base::Optional<bool> window_button_visibility_;

  // Simple (pre-Lion) Fullscreen Settings
  bool always_simple_fullscreen_ = false;
  bool is_simple_fullscreen_ = false;
  bool was_maximizable_ = false;
  bool was_movable_ = false;
  NSRect original_frame_;
  NSInteger original_level_;
  NSUInteger simple_fullscreen_mask_;

  base::scoped_nsobject<NSColor> background_color_before_vibrancy_;
  bool transparency_before_vibrancy_ = false;

  // The presentation options before entering simple fullscreen mode.
  NSApplicationPresentationOptions simple_fullscreen_options_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowMac);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NATIVE_WINDOW_MAC_H_
