// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_H_
#define ATOM_BROWSER_NATIVE_WINDOW_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "atom/browser/native_window_observer.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/supports_user_data.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/app_window/size_constraints.h"
#include "ui/views/widget/widget_delegate.h"

class SkRegion;

namespace content {
struct NativeWebKeyboardEvent;
}

namespace gfx {
class Image;
class Point;
class Rect;
class RectF;
class Size;
}  // namespace gfx

namespace mate {
class Dictionary;
class PersistentDictionary;
}  // namespace mate

namespace atom {

class AtomMenuModel;
class NativeBrowserView;

struct DraggableRegion;

class NativeWindow : public base::SupportsUserData,
                     public views::WidgetDelegate {
 public:
  ~NativeWindow() override;

  // Create window with existing WebContents, the caller is responsible for
  // managing the window's live.
  static NativeWindow* Create(const mate::Dictionary& options,
                              NativeWindow* parent = nullptr);

  void InitFromOptions(const mate::Dictionary& options);

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
      const extensions::SizeConstraints& size_constraints);
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
#if defined(OS_WIN) || defined(OS_MACOSX)
  virtual void MoveTop() = 0;
#endif
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
  virtual void SetAlwaysOnTop(bool top,
                              const std::string& level = "floating",
                              int relativeLevel = 0,
                              std::string* error = nullptr) = 0;
  virtual bool IsAlwaysOnTop() = 0;
  virtual void Center() = 0;
  virtual void Invalidate() = 0;
  virtual void SetTitle(const std::string& title) = 0;
  virtual std::string GetTitle() = 0;
  virtual void FlashFrame(bool flash) = 0;
  virtual void SetSkipTaskbar(bool skip) = 0;
  virtual void SetSimpleFullScreen(bool simple_fullscreen) = 0;
  virtual bool IsSimpleFullScreen() = 0;
  virtual void SetKiosk(bool kiosk) = 0;
  virtual bool IsKiosk() = 0;
  virtual void SetBackgroundColor(SkColor color) = 0;
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
  virtual void SetMenu(AtomMenuModel* menu);
  virtual void SetParentWindow(NativeWindow* parent);
  virtual void SetBrowserView(NativeBrowserView* browser_view) = 0;
  virtual gfx::NativeView GetNativeView() const = 0;
  virtual gfx::NativeWindow GetNativeWindow() const = 0;
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() const = 0;

  // Taskbar/Dock APIs.
  enum ProgressState {
    PROGRESS_NONE,           // no progress, no marking
    PROGRESS_INDETERMINATE,  // progress, indeterminate
    PROGRESS_ERROR,          // progress, errored (red)
    PROGRESS_PAUSED,         // progress, paused (yellow)
    PROGRESS_NORMAL,         // progress, not marked (green)
  };

  virtual void SetProgressBar(double progress, const ProgressState state) = 0;
  virtual void SetOverlayIcon(const gfx::Image& overlay,
                              const std::string& description) = 0;

  // Workspace APIs.
  virtual void SetVisibleOnAllWorkspaces(bool visible,
                                         bool visibleOnFullScreen = false) = 0;

  virtual bool IsVisibleOnAllWorkspaces() = 0;

  virtual void SetAutoHideCursor(bool auto_hide);

  // Vibrancy API
  virtual void SetVibrancy(const std::string& type);

  // Touchbar API
  virtual void SetTouchBar(
      const std::vector<mate::PersistentDictionary>& items);
  virtual void RefreshTouchBarItem(const std::string& item_id);
  virtual void SetEscapeTouchBarItem(const mate::PersistentDictionary& item);

  // Native Tab API
  virtual void SelectPreviousTab();
  virtual void SelectNextTab();
  virtual void MergeAllWindows();
  virtual void MoveTabToNewWindow();
  virtual void ToggleTabBar();
  virtual bool AddTabbedWindow(NativeWindow* window);

  // Returns false if unsupported.
  virtual bool SetWindowButtonVisibility(bool visible);

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

  // Converts between content bounds and window bounds.
  virtual gfx::Rect ContentBoundsToWindowBounds(
      const gfx::Rect& bounds) const = 0;
  virtual gfx::Rect WindowBoundsToContentBounds(
      const gfx::Rect& bounds) const = 0;

  base::WeakPtr<NativeWindow> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

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
  void NotifyWindowHide();
  void NotifyWindowMaximize();
  void NotifyWindowUnmaximize();
  void NotifyWindowMinimize();
  void NotifyWindowRestore();
  void NotifyWindowMove();
  void NotifyWindowWillResize(const gfx::Rect& new_bounds,
                              bool* prevent_default);
  void NotifyWindowResize();
  void NotifyWindowWillMove(const gfx::Rect& new_bounds, bool* prevent_default);
  void NotifyWindowMoved();
  void NotifyWindowScrollTouchBegin();
  void NotifyWindowScrollTouchEnd();
  void NotifyWindowSwipe(const std::string& direction);
  void NotifyWindowSheetBegin();
  void NotifyWindowSheetEnd();
  void NotifyWindowEnterFullScreen();
  void NotifyWindowLeaveFullScreen();
  void NotifyWindowEnterHtmlFullScreen();
  void NotifyWindowLeaveHtmlFullScreen();
  void NotifyWindowAlwaysOnTopChanged();
  void NotifyWindowExecuteWindowsCommand(const std::string& command);
  void NotifyTouchBarItemInteraction(const std::string& item_id,
                                     const base::DictionaryValue& details);
  void NotifyNewWindowForTab();

#if defined(OS_WIN)
  void NotifyWindowMessage(UINT message, WPARAM w_param, LPARAM l_param);
#endif

  void AddObserver(NativeWindowObserver* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(NativeWindowObserver* obs) {
    observers_.RemoveObserver(obs);
  }

  views::Widget* widget() const { return widget_.get(); }
  views::View* content_view() const { return content_view_; }

  bool has_frame() const { return has_frame_; }
  void set_has_frame(bool has_frame) { has_frame_ = has_frame; }

  bool transparent() const { return transparent_; }
  bool enable_larger_than_screen() const { return enable_larger_than_screen_; }

  NativeBrowserView* browser_view() const { return browser_view_; }
  NativeWindow* parent() const { return parent_; }
  bool is_modal() const { return is_modal_; }

 protected:
  NativeWindow(const mate::Dictionary& options, NativeWindow* parent);

  // views::WidgetDelegate:
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;

  void set_content_view(views::View* view) { content_view_ = view; }
  void set_browser_view(NativeBrowserView* browser_view) {
    browser_view_ = browser_view;
  }

 private:
  std::unique_ptr<views::Widget> widget_;

  // The content view, weak ref.
  views::View* content_view_ = nullptr;

  // Whether window has standard frame.
  bool has_frame_ = true;

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
  NativeBrowserView* browser_view_ = nullptr;

  // Observers of this window.
  base::ObserverList<NativeWindowObserver> observers_;

  base::WeakPtrFactory<NativeWindow> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindow);
};

// This class provides a hook to get a NativeWindow from a WebContents.
class NativeWindowRelay
    : public content::WebContentsUserData<NativeWindowRelay> {
 public:
  explicit NativeWindowRelay(base::WeakPtr<NativeWindow> window);
  ~NativeWindowRelay() override;

  static const void* UserDataKey() {
    return content::WebContentsUserData<NativeWindowRelay>::UserDataKey();
  }

  const void* key;
  base::WeakPtr<NativeWindow> window;

 private:
  friend class content::WebContentsUserData<NativeWindow>;
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_H_
