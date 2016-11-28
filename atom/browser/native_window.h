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
#include "atom/browser/ui/accelerator_util.h"
#include "atom/browser/ui/atom_menu_model.h"
#include "base/cancelable_callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/supports_user_data.h"
#include "content/public/browser/readback_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/app_window/size_constraints.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "native_mate/constructor.h"

class SkRegion;

namespace brightray {
class InspectableWebContents;
}

namespace content {
struct NativeWebKeyboardEvent;
}

namespace gfx {
class Point;
class Rect;
class Size;
}

namespace mate {
class Dictionary;
}

namespace atom {

struct DraggableRegion;

class NativeWindow : public base::SupportsUserData,
                     public content::WebContentsObserver {
 public:
  ~NativeWindow() override;

  // Create window with existing WebContents, the caller is responsible for
  // managing the window's live.
  static NativeWindow* Create(
      brightray::InspectableWebContents* inspectable_web_contents,
      const mate::Dictionary& options,
      NativeWindow* parent = nullptr);

  // Find a window from its WebContents
  static NativeWindow* FromWebContents(content::WebContents* web_contents);

  void InitFromOptions(const mate::Dictionary& options);

  virtual void Close() = 0;
  virtual void CloseImmediately() = 0;
  virtual bool IsClosed() const { return is_closed_; }
  virtual void Focus(bool focus) = 0;
  virtual bool IsFocused() = 0;
  virtual void Show() = 0;
  virtual void ShowInactive() = 0;
  virtual void Hide() = 0;
  virtual bool IsVisible() = 0;
  virtual bool IsEnabled() = 0;
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
  virtual void SetSizeConstraints(
      const extensions::SizeConstraints& size_constraints);
  virtual extensions::SizeConstraints GetSizeConstraints();
  virtual void SetContentSizeConstraints(
      const extensions::SizeConstraints& size_constraints);
  virtual extensions::SizeConstraints GetContentSizeConstraints();
  virtual void SetMinimumSize(const gfx::Size& size);
  virtual gfx::Size GetMinimumSize();
  virtual void SetMaximumSize(const gfx::Size& size);
  virtual gfx::Size GetMaximumSize();
  virtual void SetSheetOffset(const double offsetX, const double offsetY);
  virtual double GetSheetOffsetX();
  virtual double GetSheetOffsetY();
  virtual void SetResizable(bool resizable) = 0;
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
  virtual void SetKiosk(bool kiosk) = 0;
  virtual bool IsKiosk() = 0;
  virtual void SetBackgroundColor(const std::string& color_name) = 0;
  virtual void SetHasShadow(bool has_shadow) = 0;
  virtual bool HasShadow() = 0;
  virtual void SetRepresentedFilename(const std::string& filename);
  virtual std::string GetRepresentedFilename();
  virtual void SetDocumentEdited(bool edited);
  virtual bool IsDocumentEdited();
  virtual void SetIgnoreMouseEvents(bool ignore) = 0;
  virtual void SetContentProtection(bool enable) = 0;
  virtual void SetFocusable(bool focusable);
  virtual void SetMenu(AtomMenuModel* menu);
  virtual void SetParentWindow(NativeWindow* parent);
  virtual gfx::NativeWindow GetNativeWindow() = 0;
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() = 0;

  // Taskbar/Dock APIs.
  enum ProgressState {
    PROGRESS_NONE,               // no progress, no marking
    PROGRESS_INDETERMINATE,      // progress, indeterminate
    PROGRESS_ERROR,              // progress, errored (red)
    PROGRESS_PAUSED,             // progress, paused (yellow)
    PROGRESS_NORMAL,             // progress, not marked (green)
  };

  virtual void SetProgressBar(double progress,
                              const ProgressState state) = 0;
  virtual void SetOverlayIcon(const gfx::Image& overlay,
                              const std::string& description) = 0;

  // Workspace APIs.
  virtual void SetVisibleOnAllWorkspaces(bool visible) = 0;
  virtual bool IsVisibleOnAllWorkspaces() = 0;

  virtual void SetAutoHideCursor(bool auto_hide);

  // Vibrancy API
  virtual void SetVibrancy(const std::string& type);

  // Touchbar API
  virtual void DestroyTouchBar();
  virtual void SetTouchBar(mate::Arguments* args);

  // Webview APIs.
  virtual void FocusOnWebView();
  virtual void BlurWebView();
  virtual bool IsWebViewFocused();

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

  base::WeakPtr<NativeWindow> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // Requests the WebContents to close, can be cancelled by the page.
  virtual void RequestToClosePage();

  // Methods called by the WebContents.
  virtual void CloseContents(content::WebContents* source);
  virtual void RendererUnresponsive(content::WebContents* source);
  virtual void RendererResponsive(content::WebContents* source);
  virtual void HandleKeyboardEvent(
      content::WebContents*,
      const content::NativeWebKeyboardEvent& event) {}

  // Public API used by platform-dependent delegates and observers to send UI
  // related notifications.
  void NotifyWindowClosed();
  void NotifyWindowBlur();
  void NotifyWindowFocus();
  void NotifyWindowShow();
  void NotifyWindowHide();
  void NotifyWindowMaximize();
  void NotifyWindowUnmaximize();
  void NotifyWindowMinimize();
  void NotifyWindowRestore();
  void NotifyWindowMove();
  void NotifyWindowResize();
  void NotifyWindowMoved();
  void NotifyWindowScrollTouchBegin();
  void NotifyWindowScrollTouchEnd();
  void NotifyWindowScrollTouchEdge();
  void NotifyWindowSwipe(const std::string& direction);
  void NotifyWindowEnterFullScreen();
  void NotifyWindowLeaveFullScreen();
  void NotifyWindowEnterHtmlFullScreen();
  void NotifyWindowLeaveHtmlFullScreen();
  void NotifyWindowExecuteWindowsCommand(const std::string& command);
  void NotifyTouchBarItemInteraction(const std::string& item_type, const std::vector<std::string>& args);

  #if defined(OS_WIN)
  void NotifyWindowMessage(UINT message, WPARAM w_param, LPARAM l_param);
  #endif

  void AddObserver(NativeWindowObserver* obs) {
    observers_.AddObserver(obs);
  }
  void RemoveObserver(NativeWindowObserver* obs) {
    observers_.RemoveObserver(obs);
  }

  brightray::InspectableWebContents* inspectable_web_contents() const {
    return inspectable_web_contents_;
  }

  bool has_frame() const { return has_frame_; }
  void set_has_frame(bool has_frame) { has_frame_ = has_frame; }

  bool transparent() const { return transparent_; }
  SkRegion* draggable_region() const { return draggable_region_.get(); }
  bool enable_larger_than_screen() const { return enable_larger_than_screen_; }

  NativeWindow* parent() const { return parent_; }
  bool is_modal() const { return is_modal_; }

 protected:
  NativeWindow(brightray::InspectableWebContents* inspectable_web_contents,
               const mate::Dictionary& options,
               NativeWindow* parent);

  // Convert draggable regions in raw format to SkRegion format. Caller is
  // responsible for deleting the returned SkRegion instance.
  std::unique_ptr<SkRegion> DraggableRegionsToSkRegion(
      const std::vector<DraggableRegion>& regions);

  // Converts between content bounds and window bounds.
  virtual gfx::Rect ContentBoundsToWindowBounds(const gfx::Rect& bounds) = 0;
  virtual gfx::Rect WindowBoundsToContentBounds(const gfx::Rect& bounds) = 0;

  // Called when the window needs to update its draggable region.
  virtual void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions);

  // content::WebContentsObserver:
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void BeforeUnloadDialogCancelled() override;
  void DidFirstVisuallyNonEmptyPaint() override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  // Schedule a notification unresponsive event.
  void ScheduleUnresponsiveEvent(int ms);

  // Dispatch unresponsive event to observers.
  void NotifyWindowUnresponsive();

  // Dispatch ReadyToShow event to observers.
  void NotifyReadyToShow();

  // Whether window has standard frame.
  bool has_frame_;

  // Whether window is transparent.
  bool transparent_;

  // For custom drag, the whole window is non-draggable and the draggable region
  // has to been explicitly provided.
  std::unique_ptr<SkRegion> draggable_region_;  // used in custom drag.

  // Minimum and maximum size, stored as content size.
  extensions::SizeConstraints size_constraints_;

  // Whether window can be resized larger than screen.
  bool enable_larger_than_screen_;

  // The windows has been closed.
  bool is_closed_;

  // Closure that would be called when window is unresponsive when closing,
  // it should be cancelled when we can prove that the window is responsive.
  base::CancelableClosure window_unresposive_closure_;

  // Used to display sheets at the appropriate horizontal and vertical offsets
  // on macOS.
  double sheet_offset_x_;
  double sheet_offset_y_;

  // Used to maintain the aspect ratio of a view which is inside of the
  // content view.
  double aspect_ratio_;
  gfx::Size aspect_ratio_extraSize_;

  // The parent window, it is guaranteed to be valid during this window's life.
  NativeWindow* parent_;

  // Is this a modal window.
  bool is_modal_;

  // The page this window is viewing.
  brightray::InspectableWebContents* inspectable_web_contents_;

  // Observers of this window.
  base::ObserverList<NativeWindowObserver> observers_;

  base::WeakPtrFactory<NativeWindow> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindow);
};

// This class provides a hook to get a NativeWindow from a WebContents.
class NativeWindowRelay :
    public content::WebContentsUserData<NativeWindowRelay> {
 public:
  explicit NativeWindowRelay(base::WeakPtr<NativeWindow> window)
    : key(UserDataKey()), window(window) {}

  void* key;
  base::WeakPtr<NativeWindow> window;

 private:
  friend class content::WebContentsUserData<NativeWindow>;
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_H_
