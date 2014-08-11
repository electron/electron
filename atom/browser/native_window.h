// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_H_
#define ATOM_BROWSER_NATIVE_WINDOW_H_

#include <map>
#include <string>
#include <vector>

#include "atom/browser/native_window_observer.h"
#include "atom/browser/ui/accelerator_util.h"
#include "base/cancelable_callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brightray/browser/default_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents_impl.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_observer.h"
#include "native_mate/scoped_persistent.h"

struct WebPreferences;

namespace base {
class CommandLine;
}

namespace content {
class BrowserContext;
class WebContents;
}

namespace gfx {
class Image;
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

class AtomJavaScriptDialogManager;
struct DraggableRegion;

class NativeWindow : public brightray::DefaultWebContentsDelegate,
                     public brightray::InspectableWebContentsDelegate,
                     public content::WebContentsObserver,
                     public content::NotificationObserver {
 public:
  typedef base::Callback<void(const std::vector<unsigned char>& buffer)>
      CapturePageCallback;

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
  static NativeWindow* Create(content::WebContents* web_contents,
                              const mate::Dictionary& options);

  // Create window with new WebContents, the caller is responsible for
  // managing the window's live.
  static NativeWindow* Create(const mate::Dictionary& options);

  // Find a window from its process id and routing id.
  static NativeWindow* FromRenderView(int process_id, int routing_id);

  void InitFromOptions(const mate::Dictionary& options);

  virtual void Close() = 0;
  virtual void CloseImmediately() = 0;
  virtual void Move(const gfx::Rect& pos) = 0;
  virtual void Focus(bool focus) = 0;
  virtual bool IsFocused() = 0;
  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual bool IsVisible() = 0;
  virtual void Maximize() = 0;
  virtual void Unmaximize() = 0;
  virtual bool IsMaximized() = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;
  virtual bool IsMinimized() = 0;
  virtual void SetFullscreen(bool fullscreen) = 0;
  virtual bool IsFullscreen() = 0;
  virtual void SetSize(const gfx::Size& size) = 0;
  virtual gfx::Size GetSize() = 0;
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
  virtual void SetPosition(const gfx::Point& position) = 0;
  virtual gfx::Point GetPosition() = 0;
  virtual void SetTitle(const std::string& title) = 0;
  virtual std::string GetTitle() = 0;
  virtual void FlashFrame(bool flash) = 0;
  virtual void SetSkipTaskbar(bool skip) = 0;
  virtual void SetKiosk(bool kiosk) = 0;
  virtual bool IsKiosk() = 0;
  virtual void SetRepresentedFilename(const std::string& filename);
  virtual std::string GetRepresentedFilename();
  virtual void SetDocumentEdited(bool edited);
  virtual void SetMenu(ui::MenuModel* menu);
  virtual bool IsDocumentEdited();
  virtual bool HasModalDialog();
  virtual gfx::NativeWindow GetNativeWindow() = 0;

  virtual bool IsClosed() const { return is_closed_; }
  virtual void OpenDevTools();
  virtual void CloseDevTools();
  virtual bool IsDevToolsOpened();
  virtual void InspectElement(int x, int y);

  virtual void FocusOnWebView();
  virtual void BlurWebView();
  virtual bool IsWebViewFocused();

  // Captures the page with |rect|, |callback| would be called when capturing is
  // done.
  virtual void CapturePage(const gfx::Rect& rect,
                           const CapturePageCallback& callback);

  // The same with closing a tab in a real browser.
  //
  // Should be called by platform code when user want to close the window.
  virtual void CloseWebContents();

  // Destroy the WebContents immediately.
  virtual void DestroyWebContents();

  base::WeakPtr<NativeWindow> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  content::WebContents* GetWebContents() const;
  content::WebContents* GetDevToolsWebContents() const;

  // Called when renderer process is going to be started.
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id);
  void OverrideWebkitPrefs(const GURL& url, WebPreferences* prefs);

  // Public API used by platform-dependent delegates and observers to send UI
  // related notifications.
  void NotifyWindowClosed();
  void NotifyWindowBlur();
  void NotifyWindowFocus();

  void AddObserver(NativeWindowObserver* obs) {
    observers_.AddObserver(obs);
  }

  void RemoveObserver(NativeWindowObserver* obs) {
    observers_.RemoveObserver(obs);
  }

  bool has_frame() const { return has_frame_; }

  void set_has_dialog_attached(bool has_dialog_attached) {
    has_dialog_attached_ = has_dialog_attached;
  }

 protected:
  explicit NativeWindow(content::WebContents* web_contents,
                        const mate::Dictionary& options);

  brightray::InspectableWebContentsImpl* inspectable_web_contents() const {
    return static_cast<brightray::InspectableWebContentsImpl*>(
        inspectable_web_contents_.get());
  }

  // Called when the window needs to update its draggable region.
  virtual void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) = 0;

  // Implementations of content::WebContentsDelegate.
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) OVERRIDE;
  virtual content::JavaScriptDialogManager*
      GetJavaScriptDialogManager() OVERRIDE;
  virtual void BeforeUnloadFired(content::WebContents* tab,
                                 bool proceed,
                                 bool* proceed_to_fire_unload) OVERRIDE;
  virtual void RequestToLockMouse(content::WebContents* web_contents,
                                  bool user_gesture,
                                  bool last_unlocked_by_target) OVERRIDE;
  virtual bool CanOverscrollContent() const OVERRIDE;
  virtual void ActivateContents(content::WebContents* contents) OVERRIDE;
  virtual void DeactivateContents(content::WebContents* contents) OVERRIDE;
  virtual void MoveContents(content::WebContents* source,
                            const gfx::Rect& pos) OVERRIDE;
  virtual void CloseContents(content::WebContents* source) OVERRIDE;
  virtual bool IsPopupOrPanel(
      const content::WebContents* source) const OVERRIDE;
  virtual void RendererUnresponsive(content::WebContents* source) OVERRIDE;
  virtual void RendererResponsive(content::WebContents* source) OVERRIDE;

  // Implementations of content::WebContentsObserver.
  virtual void BeforeUnloadFired(const base::TimeTicks& proceed_time) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // Implementations of content::NotificationObserver.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Implementations of brightray::InspectableWebContentsDelegate.
  virtual void DevToolsSaveToFile(const std::string& url,
                                  const std::string& content,
                                  bool save_as) OVERRIDE;
  virtual void DevToolsAppendToFile(const std::string& url,
                                    const std::string& content) OVERRIDE;

  // Whether window has standard frame.
  bool has_frame_;

  // Window icon.
  scoped_ptr<gfx::Image> icon_;

 private:
  // Schedule a notification unresponsive event.
  void ScheduleUnresponsiveEvent(int ms);

  // Dispatch unresponsive event to observers.
  void NotifyWindowUnresponsive();

  // Call a function in devtools.
  void CallDevToolsFunction(const std::string& function_name,
                            const base::Value* arg1 = NULL,
                            const base::Value* arg2 = NULL,
                            const base::Value* arg3 = NULL);

  // Called when CapturePage has done.
  void OnCapturePageDone(const CapturePageCallback& callback,
                         bool succeed,
                         const SkBitmap& bitmap);

  // Notification manager.
  content::NotificationRegistrar registrar_;

  // Observers of this window.
  ObserverList<NativeWindowObserver> observers_;

  // The windows has been closed.
  bool is_closed_;

  // The security token of iframe.
  std::string node_integration_;

  // There is a dialog that has been attached to window.
  bool has_dialog_attached_;

  // Closure that would be called when window is unresponsive when closing,
  // it should be cancelled when we can prove that the window is responsive.
  base::CancelableClosure window_unresposive_closure_;

  // Web preferences.
  mate::ScopedPersistent<v8::Object> web_preferences_;

  // Page's default zoom factor.
  double zoom_factor_;

  base::WeakPtrFactory<NativeWindow> weak_factory_;

  scoped_ptr<AtomJavaScriptDialogManager> dialog_manager_;

  // Notice that inspectable_web_contents_ must be placed after dialog_manager_,
  // so we can make sure inspectable_web_contents_ is destroyed before
  // dialog_manager_, otherwise a crash would happen.
  scoped_ptr<brightray::InspectableWebContents> inspectable_web_contents_;

  // Maps url to file path, used by the file requests sent from devtools.
  typedef std::map<std::string, base::FilePath> PathsMap;
  PathsMap saved_files_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindow);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_H_
