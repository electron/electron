// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/supports_user_data.h"
#include "content/public/browser/desktop_media_id.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/app_window/size_constraints.h"
#include "shell/browser/native_window_observer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/views/widget/widget_delegate.h"

namespace base {
class DictionaryValue;
}

namespace content {
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
class NativeBrowserView;

#if BUILDFLAG(IS_MAC)
typedef NSView* NativeWindowHandle;
#else
typedef gfx::AcceleratedWidget NativeWindowHandle;
#endif

class NativeWindow : public base::SupportsUserData,
                     public views::WidgetDelegate {
 public:
  ~NativeWindow() override;

  // disable copy
  NativeWindow(const NativeWindow&) = delete;
  NativeWindow& operator=(const NativeWindow&) = delete;

  // Create window with existing WebContents, the caller is responsible for
  // managing the window's live.
  static NativeWindow* Create(const gin_helper::Dictionary& options,
                              NativeWindow* parent = nullptr);

  void InitFromOptions(const gin_helper::Dictionary& options);

  virtual void SetContentView(views::View* view) = 0;

  virtual void Close() = 0;
  virtual void CloseImmediately() = 0;
  virtual bool IsClosed() const;
  virtual void Focus(bool focus) = 0;
  virtual bool IsFocused() = 0;
  virtual void Show() = 0;
  virtual void ShowInactive() = 0;
  virtual void Hide() = 0;
  virtual bool IsVisible() = 0;
  virtual bool IsEnabled() = 0;
  virtual void SetEnabled(bool enable) = 0;
  virtual void Maximize() = 0;
  virtual void Unmaximize() = 0;
  virtual bool IsMaximized() = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;
  virtual bool IsMinimized() = 0;
  virtual void SetFullScreen(bool fullscreen) = 0;
  virtual bool IsFullscreen() const = 0;
  virtual void SetBounds(const gfx::Rect& bounds, bool animate = false) = 0;
  virtual gfx::Rect GetBounds() = 0;
  virtual void SetSize(const gfx::Size& size, bool animate = false);
  virtual gfx::Size GetSize();
  virtual void SetPosition(const gfx::Point& position, bool animate = false);
  virtual gfx::Point GetPosition();
  virtual void SetContentSize(const gfx::Size& size, bool animate = false);
  virtual gfx::Size GetContentSize();
  virtual void SetContentBounds(const gfx::Rect& bounds, bool animate = false);
  virtual gfx::Rect GetContentBounds();
  virtual bool IsNormal();
  virtual gfx::Rect GetNormalBounds() = 0;
  virtual void SetSizeConstraints(
      const extensions::SizeConstraints& window_constraints);
  virtual extensions::SizeConstraints GetSizeConstraints() const;
  virtual void SetContentSizeConstraints(
      const extensions::SizeConstraints& size_constraints);
  virtual extensions::SizeConstraints GetContentSizeConstraints() const;
  virtual void SetMinimumSize(const gfx::Size& size);
  virtual gfx::Size GetMinimumSize() const;
  virtual void SetMaximumSize(const gfx::Size& size);
  virtual gfx::Size GetMaximumSize() const;
  virtual gfx::Size GetContentMinimumSize() const;
  virtual gfx::Size GetContentMaximumSize() const;
  virtual void SetSheetOffset(const double offsetX, const double offsetY);
  virtual double GetSheetOffsetX();
  virtual double GetSheetOffsetY();
  virtual void SetResizable(bool resizable) = 0;
  virtual bool MoveAbove(const std::string& sourceId) = 0;
  virtual void MoveTop() = 0;
  virtual bool IsResizable() = 0;
  virtual void SetMovable(bool movable) = 0;
  virtual bool IsMovable() = 0;
  virtual void SetMinimizable(bool minimizable) = 0;
  virtual bool IsMinimizable() = 0;
  virtual void SetMaximizable(bool maximizable) = 0;
  virtual bool IsMaximizable() = 0;
  virtual void SetFullScreenable(bool fullscreenable) = 0;
  virtual bool IsFullScreenable() = 0;
  virtual void SetClosable(bool closable) = 0;
  virtual bool IsClosable() = 0;
  virtual void SetAlwaysOnTop(ui::ZOrderLevel z_order,
                              const std::string& level = "floating",
                              int relativeLevel = 0) = 0;
  virtual ui::ZOrderLevel GetZOrderLevel() = 0;
  virtual void Center() = 0;
  virtual void Invalidate() = 0;
  virtual void SetTitle(const std::string& title) = 0;
  virtual std::string GetTitle() = 0;
#if BUILDFLAG(IS_MAC)
  virtual std::string GetAlwaysOnTopLevel() = 0;
  virtual void SetActive(bool is_key) = 0;
  virtual bool IsActive() const = 0;
#endif

  // Ability to augment the window title for the screen readers.
  void SetAccessibleTitle(const std::string& title);
  std::string GetAccessibleTitle();

  virtual void FlashFrame(bool flash) = 0;
  virtual void SetSkipTaskbar(bool skip) = 0;
  virtual void SetExcludedFromShownWindowsMenu(bool excluded) = 0;
  virtual bool IsExcludedFromShownWindowsMenu() = 0;
  virtual void SetSimpleFullScreen(bool simple_fullscreen) = 0;
  virtual bool IsSimpleFullScreen() = 0;
  virtual void SetKiosk(bool kiosk) = 0;
  virtual bool IsKiosk() = 0;
  virtual bool IsTabletMode() const;
  virtual void SetBackgroundColor(SkColor color) = 0;
  virtual SkColor GetBackgroundColor() = 0;
  virtual void SetHasShadow(bool has_shadow) = 0;
  virtual bool HasShadow() = 0;
  virtual void SetOpacity(const double opacity) = 0;
  virtual double GetOpacity() = 0;
  virtual void SetRepresentedFilename(const std::string& filename);
  virtual std::string GetRepresentedFilename();
  virtual void SetDocumentEdited(bool edited);
  virtual bool IsDocumentEdited();
  virtual void SetIgnoreMouseEvents(bool ignore, bool forward) = 0;
  virtual void SetContentProtection(bool enable) = 0;
  virtual void SetFocusable(bool focusable);
  virtual bool IsFocusable();
  virtual void SetMenu(ElectronMenuModel* menu);
  virtual void SetParentWindow(NativeWindow* parent);
  virtual void AddBrowserView(NativeBrowserView* browser_view) = 0;
  virtual void RemoveBrowserView(NativeBrowserView* browser_view) = 0;
  virtual void SetTopBrowserView(NativeBrowserView* browser_view) = 0;
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

  virtual bool IsVisibleOnAllWorkspaces() = 0;

  virtual void SetAutoHideCursor(bool auto_hide);

  // Vibrancy API
  virtual void SetVibrancy(const std::string& type);

  // Traffic Light API
#if BUILDFLAG(IS_MAC)
  virtual void SetWindowButtonVisibility(bool visible) = 0;
  virtual bool GetWindowButtonVisibility() const = 0;
  virtual void SetTrafficLightPosition(absl::optional<gfx::Point> position) = 0;
  virtual absl::optional<gfx::Point> GetTrafficLightPosition() const = 0;
  virtual void RedrawTrafficLights() = 0;
  virtual void UpdateFrame() = 0;
#endif

  // Touchbar API
  virtual void SetTouchBar(std::vector<gin_helper::PersistentDictionary> items);
  virtual void RefreshTouchBarItem(const std::string& item_id);
  virtual void SetEscapeTouchBarItem(gin_helper::PersistentDictionary item);

  // Native Tab API
  virtual void SelectPreviousTab();
  virtual void SelectNextTab();
  virtual void MergeAllWindows();
  virtual void MoveTabToNewWindow();
  virtual void ToggleTabBar();
  virtual bool AddTabbedWindow(NativeWindow* window);

  // Toggle the menu bar.
  virtual void SetAutoHideMenuBar(bool auto_hide);
  virtual bool IsMenuBarAutoHide();
  virtual void SetMenuBarVisibility(bool visible);
  virtual bool IsMenuBarVisible();

  // Set the aspect ratio when resizing window.
  double GetAspectRatio();
  gfx::Size GetAspectRatioExtraSize();
  virtual void SetAspectRatio(double aspect_ratio, const gfx::Size& extra_size);

  // File preview APIs.
  virtual void PreviewFile(const std::string& path,
                           const std::string& display_name);
  virtual void CloseFilePreview();

  virtual void SetGTKDarkThemeEnabled(bool use_dark_theme) {}

  // Converts between content bounds and window bounds.
  virtual gfx::Rect ContentBoundsToWindowBounds(
      const gfx::Rect& bounds) const = 0;
  virtual gfx::Rect WindowBoundsToContentBounds(
      const gfx::Rect& bounds) const = 0;

  base::WeakPtr<NativeWindow> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  virtual gfx::Rect GetWindowControlsOverlayRect();
  virtual void SetWindowControlsOverlayRect(const gfx::Rect& overlay_rect);

  // Methods called by the WebContents.
  virtual void HandleKeyboardEvent(
      content::WebContents*,
      const content::NativeWebKeyboardEvent& event) {}

  // Public API used by platform-dependent delegates and observers to send UI
  // related notifications.
  void NotifyWindowRequestPreferredWith(int* width);
  void NotifyWindowCloseButtonClicked();
  void NotifyWindowClosed();
  void NotifyWindowEndSession();
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
                              const gfx::ResizeEdge& edge,
                              bool* prevent_default);
  void NotifyWindowResize();
  void NotifyWindowResized();
  void NotifyWindowWillMove(const gfx::Rect& new_bounds, bool* prevent_default);
  void NotifyWindowMoved();
  void NotifyWindowScrollTouchBegin();
  void NotifyWindowScrollTouchEnd();
  void NotifyWindowSwipe(const std::string& direction);
  void NotifyWindowRotateGesture(float rotation);
  void NotifyWindowSheetBegin();
  void NotifyWindowSheetEnd();
  virtual void NotifyWindowEnterFullScreen();
  virtual void NotifyWindowLeaveFullScreen();
  void NotifyWindowEnterHtmlFullScreen();
  void NotifyWindowLeaveHtmlFullScreen();
  void NotifyWindowAlwaysOnTopChanged();
  void NotifyWindowExecuteAppCommand(const std::string& command);
  void NotifyTouchBarItemInteraction(const std::string& item_id,
                                     const base::DictionaryValue& details);
  void NotifyNewWindowForTab();
  void NotifyWindowSystemContextMenu(int x, int y, bool* prevent_default);
  void NotifyLayoutWindowControlsOverlay();

#if BUILDFLAG(IS_WIN)
  void NotifyWindowMessage(UINT message, WPARAM w_param, LPARAM l_param);
#endif

  void AddObserver(NativeWindowObserver* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(NativeWindowObserver* obs) {
    observers_.RemoveObserver(obs);
  }

  views::Widget* widget() const { return widget_.get(); }
  views::View* content_view() const { return content_view_; }

  enum class TitleBarStyle {
    kNormal,
    kHidden,
    kHiddenInset,
    kCustomButtonsOnHover,
  };
  TitleBarStyle title_bar_style() const { return title_bar_style_; }
  int titlebar_overlay_height() const { return titlebar_overlay_height_; }

  bool has_frame() const { return has_frame_; }
  void set_has_frame(bool has_frame) { has_frame_ = has_frame; }

  bool has_client_frame() const { return has_client_frame_; }
  bool transparent() const { return transparent_; }
  bool enable_larger_than_screen() const { return enable_larger_than_screen_; }

  NativeWindow* parent() const { return parent_; }
  bool is_modal() const { return is_modal_; }

  std::list<NativeBrowserView*> browser_views() const { return browser_views_; }

  int32_t window_id() const { return next_id_; }

 protected:
  NativeWindow(const gin_helper::Dictionary& options, NativeWindow* parent);

  // views::WidgetDelegate:
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  std::u16string GetAccessibleWindowTitle() const override;

  void set_content_view(views::View* view) { content_view_ = view; }

  void add_browser_view(NativeBrowserView* browser_view) {
    browser_views_.push_back(browser_view);
  }
  void remove_browser_view(NativeBrowserView* browser_view) {
    browser_views_.remove_if(
        [&browser_view](NativeBrowserView* n) { return (n == browser_view); });
  }

  // The boolean parsing of the "titleBarOverlay" option
  bool titlebar_overlay_ = false;

  // The custom height parsed from the "height" option in a Object
  // "titleBarOverlay"
  int titlebar_overlay_height_ = 0;

  // The "titleBarStyle" option.
  TitleBarStyle title_bar_style_ = TitleBarStyle::kNormal;

 private:
  std::unique_ptr<views::Widget> widget_;

  static int32_t next_id_;

  // The content view, weak ref.
  views::View* content_view_ = nullptr;

  // Whether window has standard frame.
  bool has_frame_ = true;

  // Whether window has standard frame, but it's drawn by Electron (the client
  // application) instead of the OS. Currently only has meaning on Linux for
  // Wayland hosts.
  bool has_client_frame_ = false;

  // Whether window is transparent.
  bool transparent_ = false;

  // Minimum and maximum size, stored as content size.
  extensions::SizeConstraints size_constraints_;

  // Whether window can be resized larger than screen.
  bool enable_larger_than_screen_ = false;

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
  NativeWindow* parent_ = nullptr;

  // Is this a modal window.
  bool is_modal_ = false;

  // The browser view layer.
  std::list<NativeBrowserView*> browser_views_;

  // Observers of this window.
  base::ObserverList<NativeWindowObserver> observers_;

  // Accessible title.
  std::u16string accessible_title_;

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
