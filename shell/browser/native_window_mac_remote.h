// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_MAC_REMOTE_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_MAC_REMOTE_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "electron/shell/common/app_shim.mojom-forward.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/message_box.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/widget/widget_observer.h"
#include "v8/include/cppgc/persistent.h"

namespace electron::api {
class AppShimHost;
}  // namespace electron::api

namespace remote_cocoa {
class ApplicationHost;
namespace mojom {
class NativeWidgetNSWindow;
}  // namespace mojom
}  // namespace remote_cocoa

namespace electron {

class RootViewMac;

// A window whose NSWindow lives in an app shim process (see
// api::AppShimHost), so the Dock, Mission Control and the window server
// attribute it to the shim instead of this app. There is no NSWindow of it in
// this process to message, so everything goes through views::Widget, which
// forwards to the shim over remote_cocoa's Mojo interfaces. What those
// interfaces have no call for is not supported and is a no-op here; see
// docs/api/app-shim-host.md for the list.
class NativeWindowMacRemote : public NativeWindow,
                              private views::WidgetObserver {
 public:
  // The application host of the options' `appShim`, or null when there is
  // none or it has disconnected.
  static remote_cocoa::ApplicationHost* ApplicationHostFromOptions(
      const gin_helper::Dictionary& options);

  NativeWindowMacRemote(int32_t base_window_id,
                        const gin_helper::Dictionary& options,
                        NativeWindow* parent,
                        remote_cocoa::ApplicationHost* application_host);
  ~NativeWindowMacRemote() override;

  // NativeWindow:
  void OnTitleChanged() override;
  void SetContentView(views::View* view) override;
  void Close() override;
  void CloseImmediately() override;
  void Focus(bool focus) override;
  bool IsFocused() const override;
  void Show() override;
  void ShowInactive() override;
  void Hide() override;
  bool IsVisible() const override;
  bool IsEnabled() const override;
  void SetEnabled(bool enable) override;
  void Maximize() override;
  void Unmaximize() override;
  bool IsMaximized() const override;
  void Minimize() override;
  void Restore() override;
  bool IsMinimized() const override;
  void SetFullScreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetBounds(const gfx::Rect& bounds, bool animate) override;
  gfx::Rect GetBounds() const override;
  gfx::Rect GetNormalBounds() const override;
  void SetSizeConstraints(
      const extensions::SizeConstraints& window_constraints) override;
  void SetContentSizeConstraints(
      const extensions::SizeConstraints& size_constraints) override;
  void SetResizable(bool resizable) override;
  bool MoveAbove(const std::string& sourceId) override;
  void MoveTop() override;
  bool IsResizable() const override;
  void SetMovable(bool movable) override;
  bool IsMovable() const override;
  void SetMinimizable(bool minimizable) override;
  bool IsMinimizable() const override;
  void SetMaximizable(bool maximizable) override;
  bool IsMaximizable() const override;
  void SetFullScreenable(bool fullscreenable) override;
  bool IsFullScreenable() const override;
  void SetClosable(bool closable) override;
  bool IsClosable() const override;
  void SetAlwaysOnTop(ui::ZOrderLevel z_order,
                      const std::string& level,
                      int relative_level) override;
  std::string GetAlwaysOnTopLevel() const override;
  ui::ZOrderLevel GetZOrderLevel() const override;
  void Center() override;
  void Invalidate() override;
  bool IsActive() const override;
  void SetActive(bool is_key) override;
  void RemoveChildFromParentWindow() override;
  void RemoveChildWindow(NativeWindow* child) override;
  void AttachChildren() override;
  void DetachChildren() override;
  void FlashFrame(bool flash) override;
  void SetSkipTaskbar(bool skip) override;
  void SetExcludedFromShownWindowsMenu(bool excluded) override;
  bool IsExcludedFromShownWindowsMenu() const override;
  void SetSimpleFullScreen(bool simple_fullscreen) override;
  bool IsSimpleFullScreen() const override;
  void SetKiosk(bool kiosk) override;
  bool IsKiosk() const override;
  void SetBackgroundColor(SkColor color) override;
  SkColor GetBackgroundColor() const override;
  void SetHasShadow(bool has_shadow) override;
  bool HasShadow() const override;
  void SetOpacity(const double opacity) override;
  double GetOpacity() const override;
  void SetIgnoreMouseEvents(bool ignore, bool forward) override;
  void SetContentProtection(bool enable) override;
  bool IsContentProtected() const override;
  content::DesktopMediaID GetDesktopMediaID() const override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  gfx::AcceleratedWidget GetAcceleratedWidget() const override;
  NativeWindowHandle GetNativeWindowHandle() const override;
  void SetProgressBar(double progress, const ProgressState state) override;
  void SetOverlayIcon(const gfx::Image& overlay,
                      const std::string& description) override;
  void SetVisibleOnAllWorkspaces(bool visible,
                                 bool visibleOnFullScreen,
                                 bool skipTransformProcessType) override;
  bool IsVisibleOnAllWorkspaces() const override;
  void SetWindowButtonVisibility(bool visible) override;
  bool GetWindowButtonVisibility() const override;
  void SetWindowButtonPosition(std::optional<gfx::Point> position) override;
  std::optional<gfx::Point> GetWindowButtonPosition() const override;
  void RedrawTrafficLights() override;
  void SetVibrancy(const std::string& type, int duration) override;
  void SelectPreviousTab() override;
  void SelectNextTab() override;
  void ShowAllTabs() override;
  void MergeAllWindows() override;
  void MoveTabToNewWindow() override;
  void ToggleTabBar() override;
  bool AddTabbedWindow(NativeWindow* window) override;
  std::optional<std::string> GetTabbingIdentifier() const override;
  void PreviewFile(const std::string& path,
                   const std::string& display_name) override;
  void CloseFilePreview() override;
  void UpdateFrame() override;
  bool IsHiddenInMissionControl() const override;
  void SetHiddenInMissionControl(bool hidden) override;
  void SetAspectRatio(double aspect_ratio,
                      const gfx::Size& extra_size) override;
  gfx::Rect ContentBoundsToWindowBounds(const gfx::Rect& bounds) const override;
  gfx::Rect WindowBoundsToContentBounds(const gfx::Rect& bounds) const override;

  // The shim's process, and the id it knows this window by; null and 0 once
  // either is gone.
  mojom::AppShim* GetAppShim() const;
  uint64_t GetRemoteWindowId() const;

  // Dialogs with this window as their parent. They are sheets of the
  // NSWindow in the shim's process, which shows them; the callbacks run with
  // "cancelled" when the shim or the window goes away first.
  void ShowMessageBox(const MessageBoxSettings& settings,
                      MessageBoxCallback callback);
  // Runs a nested loop until the box is answered.
  int ShowMessageBoxSync(const MessageBoxSettings& settings);
  // False when no window in a shim is showing a box with this id.
  static bool CloseMessageBox(int id);
  void ShowOpenDialog(const file_dialog::DialogSettings& settings,
                      file_dialog::OpenDialogCallback callback);
  void ShowSaveDialog(const file_dialog::DialogSettings& settings,
                      file_dialog::SaveDialogCallback callback);

  // The window's close button, once the shim has its answer (see the client
  // view).

 protected:
  // views::WidgetDelegate:
  views::View* GetContentsView() override;
  views::ClientView* CreateClientView(views::Widget* widget) override;
  std::unique_ptr<views::FrameView> CreateFrameView(
      views::Widget* widget) override;

 private:
  // views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& bounds) override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;
  void OnWidgetShowStateChanged(views::Widget* widget) override;
  void OnWidgetUserResizeEnded(views::Widget* widget) override;
  void OnWidgetDestroyed(views::Widget* widget) override;

  int FrameViewNonClientHitTest(const gfx::Point& point);

  bool IsInAppShim() const override;

  void OnCloseButtonClicked();
  void CloseNow();
  // The remote_cocoa interface of the NSWindow in the shim's process; null
  // once it is gone.
  remote_cocoa::mojom::NativeWidgetNSWindow* GetNSWindowMojo() const;
  void PerformTabAction(mojom::AppShimTabAction action);

  // The height the window server adds above the content for this window's
  // style: its title bar's, or 0 without one.
  int TitleBarHeight() const;

  std::unique_ptr<RootViewMac> root_view_;

  // How many of the widget's notifications are on the stack. Their listeners
  // run JavaScript, and destroying the widget from inside one would free the
  // object that is notifying.
  // The shim whose process the NSWindow is in.
  cppgc::WeakPersistent<api::AppShimHost> app_shim_;

  // What the window buttons and tabs were last told, which is what the
  // getters answer with: the NSWindow is not here to ask.
  std::optional<bool> window_button_visibility_;
  std::optional<gfx::Point> traffic_light_position_;
  std::optional<std::string> tabbing_identifier_;
  // An NSVisualEffectState.
  int visual_effect_state_ = 0;

  int widget_callback_depth_ = 0;
  bool widget_destroyed_ = false;

  NSUInteger style_mask_ = 0;
  gfx::Rect last_bounds_;
  bool was_minimized_ = false;
  bool was_maximized_ = false;
  bool was_fullscreen_ = false;

  bool movable_ = true;
  bool closable_ = true;
  bool fullscreenable_ = true;
  bool is_kiosk_ = false;
  bool has_shadow_ = true;
  bool content_protected_ = false;
  bool excluded_from_windows_menu_ = false;
  double opacity_ = 1.0;
  SkColor background_color_ = SK_ColorWHITE;
  std::string always_on_top_level_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};

  base::WeakPtrFactory<NativeWindowMacRemote> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_MAC_REMOTE_H_
