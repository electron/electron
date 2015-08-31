// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_H_
#define ATOM_BROWSER_NATIVE_WINDOW_H_

#include <map>
#include <string>
#include <vector>

#include "media/base/video_frame.h"
#include "content/public/browser/render_widget_host_view_frame_subscriber.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "atom/browser/native_window_observer.h"
#include "atom/browser/ui/accelerator_util.h"
#include "base/cancelable_callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brightray/browser/inspectable_web_contents_view_delegate.h"
#include "content/public/browser/readback_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "native_mate/persistent_dictionary.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

class SkRegion;

namespace base {
class CommandLine;
}

namespace brightray {
class InspectableWebContents;
}

namespace content {
struct NativeWebKeyboardEvent;
struct WebPreferences;
}

namespace gfx {
class Point;
class Rect;
class Size;
}

namespace mate {
class Dictionary;
}

namespace ui {
class MenuModel;
}

namespace atom {

struct DraggableRegion;

class NativeWindow : public content::WebContentsObserver,
                     public brightray::InspectableWebContentsViewDelegate {
 public:
  using CapturePageCallback = base::Callback<void(const SkBitmap& bitmap)>;

  class DialogScope {
   public:
    explicit DialogScope(NativeWindow* window)
        : window_(window) {
      if (window_ != NULL)
        window_->set_has_dialog_attached(true);
    }

    ~DialogScope() {
      if (window_ != NULL)
        window_->set_has_dialog_attached(false);
    }

   private:
    NativeWindow* window_;

    DISALLOW_COPY_AND_ASSIGN(DialogScope);
  };

  virtual ~NativeWindow();

  // Create window with existing WebContents, the caller is responsible for
  // managing the window's live.
  static NativeWindow* Create(
      brightray::InspectableWebContents* inspectable_web_contents,
      const mate::Dictionary& options);

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
  virtual void Maximize() = 0;
  virtual void Unmaximize() = 0;
  virtual bool IsMaximized() = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;
  virtual bool IsMinimized() = 0;
  virtual void SetFullScreen(bool fullscreen) = 0;
  virtual bool IsFullscreen() const = 0;
  virtual void SetBounds(const gfx::Rect& bounds) = 0;
  virtual gfx::Rect GetBounds() = 0;
  virtual void SetSize(const gfx::Size& size);
  virtual gfx::Size GetSize();
  virtual void SetPosition(const gfx::Point& position);
  virtual gfx::Point GetPosition();
  virtual void SetContentSize(const gfx::Size& size) = 0;
  virtual gfx::Size GetContentSize() = 0;
  virtual void SetMinimumSize(const gfx::Size& size) = 0;
  virtual gfx::Size GetMinimumSize() = 0;
  virtual void SetMaximumSize(const gfx::Size& size) = 0;
  virtual gfx::Size GetMaximumSize() = 0;
  virtual void SetResizable(bool resizable) = 0;
  virtual bool IsResizable() = 0;
  virtual void SetAlwaysOnTop(bool top) = 0;
  virtual bool IsAlwaysOnTop() = 0;
  virtual void Center() = 0;
  virtual void SetTitle(const std::string& title) = 0;
  virtual std::string GetTitle() = 0;
  virtual void FlashFrame(bool flash) = 0;
  virtual void SetSkipTaskbar(bool skip) = 0;
  virtual void SetKiosk(bool kiosk) = 0;
  virtual bool IsKiosk() = 0;
  virtual void SetRepresentedFilename(const std::string& filename);
  virtual std::string GetRepresentedFilename();
  virtual void SetDocumentEdited(bool edited);
  virtual bool IsDocumentEdited();
  virtual void SetMenu(ui::MenuModel* menu);
  virtual bool HasModalDialog();
  virtual gfx::NativeWindow GetNativeWindow() = 0;

  // Taskbar/Dock APIs.
  virtual void SetProgressBar(double progress) = 0;
  virtual void SetOverlayIcon(const gfx::Image& overlay,
                              const std::string& description) = 0;

  // Workspace APIs.
  virtual void SetVisibleOnAllWorkspaces(bool visible) = 0;
  virtual bool IsVisibleOnAllWorkspaces() = 0;

  // Webview APIs.
  virtual void FocusOnWebView();
  virtual void BlurWebView();
  virtual bool IsWebViewFocused();

  // Captures the page with |rect|, |callback| would be called when capturing is
  // done.
  virtual void CapturePage(const gfx::Rect& rect,
                           const CapturePageCallback& callback);

  // Show popup dictionary.
  virtual void ShowDefinitionForSelection();

  // Toggle the menu bar.
  virtual void SetAutoHideMenuBar(bool auto_hide);
  virtual bool IsMenuBarAutoHide();
  virtual void SetMenuBarVisibility(bool visible);
  virtual bool IsMenuBarVisible();

  // Set the aspect ratio when resizing window.
  double GetAspectRatio();
  gfx::Size GetAspectRatioExtraSize();
  void SetAspectRatio(double aspect_ratio, const gfx::Size& extra_size);

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

  // Called when renderer process is going to be started.
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line);
  void OverrideWebkitPrefs(content::WebPreferences* prefs);

  // Public API used by platform-dependent delegates and observers to send UI
  // related notifications.
  void NotifyWindowClosed();
  void NotifyWindowBlur();
  void NotifyWindowFocus();
  void NotifyWindowMaximize();
  void NotifyWindowUnmaximize();
  void NotifyWindowMinimize();
  void NotifyWindowRestore();
  void NotifyWindowMove();
  void NotifyWindowResize();
  void NotifyWindowMoved();
  void NotifyWindowEnterFullScreen();
  void NotifyWindowLeaveFullScreen();
  void NotifyWindowEnterHtmlFullScreen();
  void NotifyWindowLeaveHtmlFullScreen();
  void NotifyWindowExecuteWindowsCommand(const std::string& command);

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
  bool transparent() const { return transparent_; }
  SkRegion* draggable_region() const { return draggable_region_.get(); }
  bool enable_larger_than_screen() const { return enable_larger_than_screen_; }
  gfx::ImageSkia icon() const { return icon_; }

  void set_has_dialog_attached(bool has_dialog_attached) {
    has_dialog_attached_ = has_dialog_attached;
  }

  void OnFrameReceived(bool result, scoped_refptr<media::VideoFrame> frame);

  void SendKeyboardEvent(blink::WebInputEvent::Type type, int modifiers, int keycode);
  void SendMouseEvent(blink::WebInputEvent::Type type, int modifiers, blink::WebMouseEvent::Button button, int x, int y, int movementX, int movementY, int clickCount);
  void SendMouseWheelEvent(int modifiers, int x, int y, bool clickCount);

  void SetOffscreenRender(bool isOffscreen);

 protected:
  NativeWindow(brightray::InspectableWebContents* inspectable_web_contents,
               const mate::Dictionary& options);

  // brightray::InspectableWebContentsViewDelegate:
  void DevToolsFocused() override;
  void DevToolsOpened() override;
  void DevToolsClosed() override;

  // content::WebContentsObserver:
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host, const GURL& validated_url) override;
  void BeforeUnloadDialogCancelled() override;
  void TitleWasSet(content::NavigationEntry* entry, bool explicit_set) override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  // Called when the window needs to update its draggable region.
  void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions);

  // Schedule a notification unresponsive event.
  void ScheduleUnresponsiveEvent(int ms);

  // Dispatch unresponsive event to observers.
  void NotifyWindowUnresponsive();

  // Called when CapturePage has done.
  void OnCapturePageDone(const CapturePageCallback& callback,
                         const SkBitmap& bitmap,
                         content::ReadbackResponse response);

  bool offscreen_;

  // Whether window has standard frame.
  bool has_frame_;

  // Whether window is transparent.
  bool transparent_;

  // For custom drag, the whole window is non-draggable and the draggable region
  // has to been explicitly provided.
  scoped_ptr<SkRegion> draggable_region_;  // used in custom drag.

  // Whether window can be resized larger than screen.
  bool enable_larger_than_screen_;

  // Window icon.
  gfx::ImageSkia icon_;

  // The windows has been closed.
  bool is_closed_;

  // Whether node integration is enabled.
  bool node_integration_;

  // There is a dialog that has been attached to window.
  bool has_dialog_attached_;

  // Closure that would be called when window is unresponsive when closing,
  // it should be cancelled when we can prove that the window is responsive.
  base::CancelableClosure window_unresposive_closure_;

  // Web preferences.
  mate::PersistentDictionary web_preferences_;

  // The script to load before page's JavaScript starts to run.
  base::FilePath preload_script_;

  // Page's default zoom factor.
  double zoom_factor_;

  // Used to maintain the aspect ratio of a view which is inside of the
  // content view.
  double aspect_ratio_;
  gfx::Size aspect_ratio_extraSize_;

  // The page this window is viewing.
  brightray::InspectableWebContents* inspectable_web_contents_;

  // Observers of this window.
  ObserverList<NativeWindowObserver> observers_;

  base::WeakPtrFactory<NativeWindow> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindow);
};

class RenderSubscriber : public content::RenderWidgetHostViewFrameSubscriber {
 public:
  RenderSubscriber(gfx::Size size, base::Callback<void(bool, scoped_refptr<media::VideoFrame>)> callback) : size_(size), callback_(callback) {}

  bool ShouldCaptureFrame(const gfx::Rect& damage_rect,
                          base::TimeTicks present_time,
                          scoped_refptr<media::VideoFrame>* storage,
                          DeliverFrameCallback* callback) override;

  base::TimeTicks last_present_time() const { return last_present_time_; }

  static void CallbackMethod(base::Callback<void(bool, scoped_refptr<media::VideoFrame>)> callback,
                             scoped_refptr<media::VideoFrame> frame,
                             base::TimeTicks present_time,
                             bool success) {
    callback.Run(success, frame);
  }

 private:
  gfx::Size size_;
  base::Callback<void(bool, scoped_refptr<media::VideoFrame>)> callback_;
  base::TimeTicks last_present_time_;
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
