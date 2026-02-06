// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_H_

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/queue.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/cstring_view.h"
#include "content/public/browser/desktop_media_id.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/app_window/size_constraints.h"
#include "shell/browser/native_window_observer.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"
#include "ui/views/widget/widget_delegate.h"

class SkRegion;
class DraggableRegionProvider;

namespace input {
struct NativeWebKeyboardEvent;
}

namespace gfx {
class Image;
class Point;
class Rect;
enum class ResizeEdge;
class Size;
}  // namespace gfx

namespace gin_helper {
class Dictionary;
class PersistentDictionary;
}  // namespace gin_helper

namespace electron {

class ElectronMenuModel;
class BackgroundThrottlingSource;

#if BUILDFLAG(IS_MAC)
using NativeWindowHandle = gfx::NativeView;
#else
using NativeWindowHandle = gfx::AcceleratedWidget;
#endif

class NativeWindow : public views::WidgetDelegate {
 public:
  ~NativeWindow() override;

  // disable copy
  NativeWindow(const NativeWindow&) = delete;
  NativeWindow& operator=(const NativeWindow&) = delete;

  // Create window with existing WebContents, the caller is responsible for
  // managing the window's live.
  static std::unique_ptr<NativeWindow> Create(
      int32_t base_window_id,
      const gin_helper::Dictionary& options,
      NativeWindow* parent = nullptr);

  [[nodiscard]] static NativeWindow* FromWidget(const views::Widget* widget);

  void InitFromOptions(const gin_helper::Dictionary& options);

  virtual void SetContentView(views::View* view) = 0;

  virtual void Close() = 0;
  virtual void CloseImmediately() = 0;
  virtual bool IsClosed() const;
  virtual void Focus(bool focus) = 0;
  virtual bool IsFocused() const = 0;
  virtual void Show() = 0;
  virtual void ShowInactive() = 0;
  virtual void Hide() = 0;
  virtual bool IsVisible() const = 0;
  virtual bool IsEnabled() const = 0;
  virtual void SetEnabled(bool enable) = 0;
  virtual void Maximize() = 0;
  virtual void Unmaximize() = 0;
  virtual bool IsMaximized() const = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;
  virtual bool IsMinimized() const = 0;
  virtual void SetFullScreen(bool fullscreen) = 0;
  virtual bool IsFullscreen() const = 0;

  virtual void SetBounds(const gfx::Rect& bounds, bool animate = false) = 0;
  virtual gfx::Rect GetBounds() const = 0;
  void SetShape(const std::vector<gfx::Rect>& rects);
  void SetSize(const gfx::Size& size, bool animate = false);
  [[nodiscard]] gfx::Size GetSize() const;

  void SetPosition(const gfx::Point& position, bool animate = false);
  [[nodiscard]] gfx::Point GetPosition() const;

  void SetContentSize(const gfx::Size& size, bool animate = false);
  virtual gfx::Size GetContentSize() const;
  void SetContentBounds(const gfx::Rect& bounds, bool animate = false);
  virtual gfx::Rect GetContentBounds() const;
  virtual bool IsNormal() const;
  virtual gfx::Rect GetNormalBounds() const = 0;
  virtual void SetSizeConstraints(
      const extensions::SizeConstraints& window_constraints);
  virtual extensions::SizeConstraints GetSizeConstraints() const;
  virtual void SetContentSizeConstraints(
      const extensions::SizeConstraints& size_constraints);
  virtual extensions::SizeConstraints GetContentSizeConstraints() const;

  void SetMinimumSize(const gfx::Size& size);
  [[nodiscard]] gfx::Size GetMinimumSize() const;

  void SetMaximumSize(const gfx::Size& size);
  [[nodiscard]] gfx::Size GetMaximumSize() const;

  [[nodiscard]] gfx::Size GetContentMinimumSize() const;
  [[nodiscard]] gfx::Size GetContentMaximumSize() const;

  void SetSheetOffset(const double offsetX, const double offsetY);
  [[nodiscard]] double GetSheetOffsetX() const;
  [[nodiscard]] double GetSheetOffsetY() const;

  virtual void SetResizable(bool resizable) = 0;
  virtual bool MoveAbove(const std::string& sourceId) = 0;
  virtual void MoveTop() = 0;
  virtual bool IsResizable() const = 0;
  virtual void SetMovable(bool movable) = 0;
  virtual bool IsMovable() const = 0;
  virtual void SetMinimizable(bool minimizable) = 0;
  virtual bool IsMinimizable() const = 0;
  virtual void SetMaximizable(bool maximizable) = 0;
  virtual bool IsMaximizable() const = 0;
  virtual void SetFullScreenable(bool fullscreenable) = 0;
  virtual bool IsFullScreenable() const = 0;
  virtual void SetClosable(bool closable) = 0;
  virtual bool IsClosable() const = 0;
  virtual void SetAlwaysOnTop(ui::ZOrderLevel z_order,
                              const std::string& level = "floating",
                              int relativeLevel = 0) = 0;
  virtual ui::ZOrderLevel GetZOrderLevel() const = 0;
  virtual void Center() = 0;
  virtual void Invalidate() = 0;
  [[nodiscard]] virtual bool IsActive() const = 0;
#if BUILDFLAG(IS_MAC)
  virtual std::string GetAlwaysOnTopLevel() const = 0;
  virtual void SetActive(bool is_key) = 0;
  virtual void RemoveChildFromParentWindow() = 0;
  virtual void RemoveChildWindow(NativeWindow* child) = 0;
  virtual void AttachChildren() = 0;
  virtual void DetachChildren() = 0;
#endif

  void SetTitle(std::string_view title);
  [[nodiscard]] std::string GetTitle() const;

  // Ability to augment the window title for the screen readers.
  void SetAccessibleTitle(const std::string& title);
  std::string GetAccessibleTitle();

  virtual void FlashFrame(bool flash) = 0;
  virtual void SetSkipTaskbar(bool skip) = 0;
  virtual void SetExcludedFromShownWindowsMenu(bool excluded) = 0;
  virtual bool IsExcludedFromShownWindowsMenu() const = 0;
  virtual void SetSimpleFullScreen(bool simple_fullscreen) = 0;
  virtual bool IsSimpleFullScreen() const = 0;
  virtual void SetKiosk(bool kiosk) = 0;
  virtual bool IsKiosk() const = 0;
  virtual bool IsTabletMode() const;
  virtual void SetBackgroundColor(SkColor color) = 0;
  virtual SkColor GetBackgroundColor() const = 0;
  virtual void InvalidateShadow() {}

  virtual void SetHasShadow(bool has_shadow) = 0;
  virtual bool HasShadow() const = 0;
  virtual void SetOpacity(const double opacity) = 0;
  virtual double GetOpacity() const = 0;
  virtual void SetRepresentedFilename(const std::string& filename) {}
  virtual std::string GetRepresentedFilename() const;
  virtual void SetDocumentEdited(bool edited) {}
  virtual bool IsDocumentEdited() const;
  virtual void SetIgnoreMouseEvents(bool ignore, bool forward) = 0;
  virtual void SetContentProtection(bool enable) = 0;
  virtual bool IsContentProtected() const = 0;
  virtual void SetFocusable(bool focusable) {}
  virtual bool IsFocusable() const;
  virtual void SetMenu(ElectronMenuModel* menu) {}
  virtual void SetParentWindow(NativeWindow* parent);
  virtual content::DesktopMediaID GetDesktopMediaID() const = 0;
  virtual gfx::NativeView GetNativeView() const = 0;
  virtual gfx::NativeWindow GetNativeWindow() const = 0;
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() const = 0;
  virtual NativeWindowHandle GetNativeWindowHandle() const = 0;

  // Taskbar/Dock APIs.
  enum class ProgressState {
    kNone,           // no progress, no marking
    kIndeterminate,  // progress, indeterminate
    kError,          // progress, errored (red)
    kPaused,         // progress, paused (yellow)
    kNormal,         // progress, not marked (green)
  };

  virtual void SetProgressBar(double progress, const ProgressState state) = 0;
  virtual void SetOverlayIcon(const gfx::Image& overlay,
                              const std::string& description) = 0;

  // Workspace APIs.
  virtual void SetVisibleOnAllWorkspaces(
      bool visible,
      bool visibleOnFullScreen = false,
      bool skipTransformProcessType = false) = 0;

  virtual bool IsVisibleOnAllWorkspaces() const = 0;

  virtual void SetAutoHideCursor(bool auto_hide) {}

  // Vibrancy API
  virtual void SetVibrancy(const std::string& type, int duration);

  const std::string& background_material() const {
    return background_material_;
  }

  virtual void SetBackgroundMaterial(const std::string& type);

  // Traffic Light API
#if BUILDFLAG(IS_MAC)
  virtual void SetWindowButtonVisibility(bool visible) = 0;
  virtual bool GetWindowButtonVisibility() const = 0;
  virtual void SetWindowButtonPosition(std::optional<gfx::Point> position) = 0;
  virtual std::optional<gfx::Point> GetWindowButtonPosition() const = 0;
  virtual void RedrawTrafficLights() = 0;
  virtual void UpdateFrame() = 0;
#endif

// whether windows should be ignored by mission control
#if BUILDFLAG(IS_MAC)
  virtual bool IsHiddenInMissionControl() const = 0;
  virtual void SetHiddenInMissionControl(bool hidden) = 0;
#endif

  // Touchbar API
  virtual void SetTouchBar(std::vector<gin_helper::PersistentDictionary> items);
  virtual void RefreshTouchBarItem(const std::string& item_id) {}
  virtual void SetEscapeTouchBarItem(gin_helper::PersistentDictionary item);

  // Native Tab API
  virtual void SelectPreviousTab() {}
  virtual void SelectNextTab() {}
  virtual void ShowAllTabs() {}
  virtual void MergeAllWindows() {}
  virtual void MoveTabToNewWindow() {}
  virtual void ToggleTabBar() {}
  virtual bool AddTabbedWindow(NativeWindow* window);
  virtual std::optional<std::string> GetTabbingIdentifier() const;

  // Toggle the menu bar.
  virtual void SetAutoHideMenuBar(bool auto_hide) {}
  virtual bool IsMenuBarAutoHide() const;
  virtual void SetMenuBarVisibility(bool visible) {}
  virtual bool IsMenuBarVisible() const;

  virtual bool IsSnapped() const;

  // Set the aspect ratio when resizing window.
  [[nodiscard]] double aspect_ratio() const { return aspect_ratio_; }
  [[nodiscard]] gfx::Size aspect_ratio_extra_size() const {
    return aspect_ratio_extraSize_;
  }
  virtual void SetAspectRatio(double aspect_ratio, const gfx::Size& extra_size);

  // File preview APIs.
  virtual void PreviewFile(const std::string& path,
                           const std::string& display_name) {}
  virtual void CloseFilePreview() {}

  virtual void SetGTKDarkThemeEnabled(bool use_dark_theme) {}

  base::WeakPtr<NativeWindow> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  virtual std::optional<gfx::Rect> GetWindowControlsOverlayRect();
  virtual void SetWindowControlsOverlayRect(const gfx::Rect& overlay_rect);

  // Methods called by the WebContents.
  virtual void HandleKeyboardEvent(content::WebContents*,
                                   const input::NativeWebKeyboardEvent& event) {
  }

  // Public API used by platform-dependent delegates and observers to send UI
  // related notifications.
  void NotifyWindowRequestPreferredWidth(int* width);
  void NotifyWindowCloseButtonClicked();
  void NotifyWindowClosed();
  void NotifyWindowQueryEndSession(const std::vector<std::string>& reasons,
                                   bool* prevent_default);
  void NotifyWindowEndSession(const std::vector<std::string>& reasons);
  void NotifyWindowBlur();
  void NotifyWindowFocus();
  void NotifyWindowShow();
  void NotifyWindowIsKeyChanged(bool is_key);
  void NotifyWindowHide();
  void NotifyWindowMaximize();
  void NotifyWindowUnmaximize();
  void NotifyWindowMinimize();
  void NotifyWindowRestore();
  void NotifyWindowMove();
  void NotifyWindowWillResize(const gfx::Rect& new_bounds,
                              gfx::ResizeEdge edge,
                              bool* prevent_default);
  void NotifyWindowResize();
  void NotifyWindowResized();
  void NotifyWindowWillMove(const gfx::Rect& new_bounds, bool* prevent_default);
  void NotifyWindowMoved();
  void NotifyWindowSwipe(const std::string& direction);
  void NotifyWindowRotateGesture(float rotation);
  void NotifyWindowSheetBegin();
  void NotifyWindowSheetEnd();
  virtual void NotifyWindowEnterFullScreen();
  virtual void NotifyWindowLeaveFullScreen();
  void NotifyWindowEnterHtmlFullScreen();
  void NotifyWindowLeaveHtmlFullScreen();
  void NotifyWindowAlwaysOnTopChanged();
  void NotifyWindowExecuteAppCommand(std::string_view command_name);
  void NotifyTouchBarItemInteraction(const std::string& item_id,
                                     base::DictValue details);
  void NotifyNewWindowForTab();
  void NotifyWindowSystemContextMenu(int x, int y, bool* prevent_default);
  void NotifyLayoutWindowControlsOverlay();

#if BUILDFLAG(IS_WIN)
  void NotifyWindowMessage(UINT message, WPARAM w_param, LPARAM l_param);
  virtual void SetAccentColor(
      std::variant<std::monostate, bool, SkColor> accent_color) = 0;
  virtual std::variant<bool, std::string> GetAccentColor() const = 0;
  virtual void UpdateWindowAccentColor(bool active) = 0;
#endif

  void AddObserver(NativeWindowObserver* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(NativeWindowObserver* obs) {
    observers_.RemoveObserver(obs);
  }

  // Handle fullscreen transitions.
  void HandlePendingFullscreenTransitions();

  constexpr void set_is_transitioning_fullscreen(const bool val) {
    is_transitioning_fullscreen_ = val;
  }

  [[nodiscard]] constexpr bool is_transitioning_fullscreen() const {
    return is_transitioning_fullscreen_;
  }

  enum class FullScreenTransitionType { kHTML, kNative, kNone };

  void set_fullscreen_transition_type(FullScreenTransitionType type) {
    fullscreen_transition_type_ = type;
  }
  FullScreenTransitionType fullscreen_transition_type() const {
    return fullscreen_transition_type_;
  }

  views::Widget* widget() const { return widget_.get(); }
  views::View* content_view() const { return content_view_; }

  enum class TitleBarStyle : uint8_t {
    kNormal,
    kHidden,
    kHiddenInset,
    kCustomButtonsOnHover,
  };

  [[nodiscard]] TitleBarStyle title_bar_style() const {
    return title_bar_style_;
  }

  bool IsWindowControlsOverlayEnabled() const {
    bool valid_titlebar_style = title_bar_style() == TitleBarStyle::kHidden
#if BUILDFLAG(IS_MAC)
                                ||
                                title_bar_style() == TitleBarStyle::kHiddenInset
#endif
        ;
    return valid_titlebar_style && titlebar_overlay_;
  }

  int titlebar_overlay_height() const { return titlebar_overlay_height_; }

  [[nodiscard]] bool has_frame() const { return has_frame_; }

  NativeWindow* parent() const { return parent_; }

  [[nodiscard]] bool is_modal() const { return is_modal_; }

  [[nodiscard]] constexpr int32_t window_id() const { return window_id_; }

  void add_child_window(NativeWindow* child) {
    child_windows_.push_back(child);
  }

  int NonClientHitTest(const gfx::Point& point);
  void AddDraggableRegionProvider(DraggableRegionProvider* provider);
  void RemoveDraggableRegionProvider(DraggableRegionProvider* provider);

  bool IsTranslucent() const;

  // Adds |source| to |background_throttling_sources_|, triggers update of
  // background throttling state.
  void AddBackgroundThrottlingSource(BackgroundThrottlingSource* source);
  // Removes |source| to |background_throttling_sources_|, triggers update of
  // background throttling state.
  void RemoveBackgroundThrottlingSource(BackgroundThrottlingSource* source);
  // Updates `ui::Compositor` background throttling state based on
  // |background_throttling_sources_|. If at least one of the sources disables
  // throttling, then throttling in the `ui::Compositor` will be disabled.
  void UpdateBackgroundThrottlingState();

  [[nodiscard]] auto base_window_id() const { return base_window_id_; }

 protected:
  NativeWindow(int32_t base_window_id,
               const gin_helper::Dictionary& options,
               NativeWindow* parent);

  void set_titlebar_overlay_height(int height) {
    titlebar_overlay_height_ = height;
  }

  [[nodiscard]] bool has_client_frame() const { return has_client_frame_; }

  [[nodiscard]] bool transparent() const { return transparent_; }

  [[nodiscard]] bool is_closed() const { return is_closed_; }

  [[nodiscard]] bool enable_larger_than_screen() const {
    return enable_larger_than_screen_;
  }

  virtual void OnTitleChanged() {}

  // Converts between content bounds and window bounds.
  virtual gfx::Rect ContentBoundsToWindowBounds(
      const gfx::Rect& bounds) const = 0;
  virtual gfx::Rect WindowBoundsToContentBounds(
      const gfx::Rect& bounds) const = 0;

  // views::WidgetDelegate:
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;

  void set_content_view(views::View* view) { content_view_ = view; }

  static inline constexpr base::cstring_view kNativeWindowKey =
      "__ELECTRON_NATIVE_WINDOW__";

  // The boolean parsing of the "titleBarOverlay" option
  bool titlebar_overlay_ = false;

  // Minimum and maximum size.
  std::optional<extensions::SizeConstraints> size_constraints_;
  // Same as above but stored as content size, we are storing 2 types of size
  // constraints because converting between them will cause rounding errors
  // on HiDPI displays on some environments.
  std::optional<extensions::SizeConstraints> content_size_constraints_;

  base::queue<bool> pending_transitions_;

  FullScreenTransitionType fullscreen_transition_type_ =
      FullScreenTransitionType::kNone;

  std::list<NativeWindow*> child_windows_;

 private:
  static bool PlatformHasClientFrame();

  std::unique_ptr<views::Widget> widget_ = std::make_unique<views::Widget>();

  static inline int32_t next_id_ = 0;
  const int32_t window_id_ = ++next_id_;

  // ID of the api::BaseWindow that owns this NativeWindow.
  const int32_t base_window_id_;

  // The "titleBarStyle" option.
  const TitleBarStyle title_bar_style_;

  // Whether window has standard frame, but it's drawn by Electron (the client
  // application) instead of the OS. Currently only has meaning on Linux for
  // Wayland hosts.
  const bool has_client_frame_ = PlatformHasClientFrame();

  // Whether window is transparent.
  const bool transparent_;

  // Whether window can be resized larger than screen.
  const bool enable_larger_than_screen_;

  // Is this a modal window.
  const bool is_modal_;

  // Whether window has standard frame.
  const bool has_frame_;

  // The content view, weak ref.
  raw_ptr<views::View> content_view_ = nullptr;

  // The custom height parsed from the "height" option in a Object
  // "titleBarOverlay"
  int titlebar_overlay_height_ = 0;

  // The windows has been closed.
  bool is_closed_ = false;

  // Used to display sheets at the appropriate horizontal and vertical offsets
  // on macOS.
  double sheet_offset_x_ = 0.0;
  double sheet_offset_y_ = 0.0;

  // Used to maintain the aspect ratio of a view which is inside of the
  // content view.
  double aspect_ratio_ = 0.0;
  gfx::Size aspect_ratio_extraSize_;

  // The parent window, it is guaranteed to be valid during this window's life.
  raw_ptr<NativeWindow> parent_ = nullptr;

  bool is_transitioning_fullscreen_ = false;

  std::list<DraggableRegionProvider*> draggable_region_providers_;

  // Observers of this window.
  base::ObserverList<NativeWindowObserver> observers_;

  absl::flat_hash_set<BackgroundThrottlingSource*>
      background_throttling_sources_;

  std::string vibrancy_;
  std::string background_material_;

  gfx::Rect overlay_rect_;

  base::WeakPtrFactory<NativeWindow> weak_factory_{this};
};

// This class provides a hook to get a NativeWindow from a WebContents.
class NativeWindowRelay
    : public content::WebContentsUserData<NativeWindowRelay> {
 public:
  static void CreateForWebContents(content::WebContents*,
                                   base::WeakPtr<NativeWindow>);

  ~NativeWindowRelay() override;

  NativeWindow* GetNativeWindow() const { return native_window_.get(); }

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  friend class content::WebContentsUserData<NativeWindow>;
  explicit NativeWindowRelay(content::WebContents* web_contents,
                             base::WeakPtr<NativeWindow> window);

  base::WeakPtr<NativeWindow> native_window_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_H_
