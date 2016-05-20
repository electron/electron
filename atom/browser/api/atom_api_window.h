// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WINDOW_H_
#define ATOM_BROWSER_API_ATOM_API_WINDOW_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "ui/gfx/image/image.h"
#include "atom/browser/api/trackable_object.h"
#include "atom/browser/native_window.h"
#include "atom/browser/native_window_observer.h"
#include "atom/common/api/atom_api_native_image.h"
#include "native_mate/handle.h"

class GURL;

namespace gfx {
class Rect;
}

namespace mate {
class Arguments;
class Dictionary;
}

namespace atom {

class NativeWindow;

namespace api {

class WebContents;

class Window : public mate::TrackableObject<Window>,
               public NativeWindowObserver {
 public:
  static mate::WrappableBase* New(v8::Isolate* isolate, mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

  // Returns the BrowserWindow object from |native_window|.
  static v8::Local<v8::Value> From(v8::Isolate* isolate,
                                   NativeWindow* native_window);

  NativeWindow* window() const { return window_.get(); }

 protected:
  Window(v8::Isolate* isolate, const mate::Dictionary& options);
  ~Window() override;

  // NativeWindowObserver:
  void WillCloseWindow(bool* prevent_default) override;
  void OnWindowClosed() override;
  void OnWindowBlur() override;
  void OnWindowFocus() override;
  void OnWindowShow() override;
  void OnWindowHide() override;
  void OnWindowMaximize() override;
  void OnWindowUnmaximize() override;
  void OnWindowMinimize() override;
  void OnWindowRestore() override;
  void OnWindowResize() override;
  void OnWindowMove() override;
  void OnWindowMoved() override;
  void OnWindowScrollTouchBegin() override;
  void OnWindowScrollTouchEnd() override;
  void OnWindowSwipe(const std::string& direction) override;
  void OnWindowEnterFullScreen() override;
  void OnWindowLeaveFullScreen() override;
  void OnWindowEnterHtmlFullScreen() override;
  void OnWindowLeaveHtmlFullScreen() override;
  void OnRendererUnresponsive() override;
  void OnRendererResponsive() override;
  void OnExecuteWindowsCommand(const std::string& command_name) override;

  #if defined(OS_WIN)
  void OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) override;
  #endif

 private:
  // APIs for NativeWindow.
  void Close();
  void Focus();
  void Blur();
  bool IsFocused();
  void Show();
  void ShowInactive();
  void Hide();
  bool IsVisible();
  void Maximize();
  void Unmaximize();
  bool IsMaximized();
  void Minimize();
  void Restore();
  bool IsMinimized();
  void SetFullScreen(bool fullscreen);
  bool IsFullscreen();
  void SetBounds(const gfx::Rect& bounds, mate::Arguments* args);
  gfx::Rect GetBounds();
  void SetSize(int width, int height, mate::Arguments* args);
  std::vector<int> GetSize();
  void SetContentSize(int width, int height, mate::Arguments* args);
  std::vector<int> GetContentSize();
  void SetMinimumSize(int width, int height);
  std::vector<int> GetMinimumSize();
  void SetMaximumSize(int width, int height);
  std::vector<int> GetMaximumSize();
  void SetSheetOffset(double offset);
  void SetResizable(bool resizable);
  bool IsResizable();
  void SetMovable(bool movable);
  bool IsMovable();
  void SetMinimizable(bool minimizable);
  bool IsMinimizable();
  void SetMaximizable(bool maximizable);
  bool IsMaximizable();
  void SetFullScreenable(bool fullscreenable);
  bool IsFullScreenable();
  void SetClosable(bool closable);
  bool IsClosable();
  void SetAlwaysOnTop(bool top);
  bool IsAlwaysOnTop();
  void Center();
  void SetPosition(int x, int y, mate::Arguments* args);
  std::vector<int> GetPosition();
  void SetTitle(const std::string& title);
  std::string GetTitle();
  void FlashFrame(bool flash);
  void SetSkipTaskbar(bool skip);
  void SetKiosk(bool kiosk);
  bool IsKiosk();
  void SetBackgroundColor(const std::string& color_name);
  void SetHasShadow(bool has_shadow);
  bool HasShadow();
  void FocusOnWebView();
  void BlurWebView();
  bool IsWebViewFocused();
  void SetRepresentedFilename(const std::string& filename);
  std::string GetRepresentedFilename();
  void SetDocumentEdited(bool edited);
  bool IsDocumentEdited();
  void SetIgnoreMouseEvents(bool ignore);
  void CapturePage(mate::Arguments* args);
  void SetProgressBar(double progress);
  void SetOverlayIcon(const gfx::Image& overlay,
                      const std::string& description);
  bool SetThumbarButtons(mate::Arguments* args);
  void SetMenu(v8::Isolate* isolate, v8::Local<v8::Value> menu);
  void SetAutoHideMenuBar(bool auto_hide);
  bool IsMenuBarAutoHide();
  void SetMenuBarVisibility(bool visible);
  bool IsMenuBarVisible();
  void SetAspectRatio(double aspect_ratio, mate::Arguments* args);
  v8::Local<v8::Value> GetNativeWindowHandle();

#if defined(OS_WIN)
  typedef base::Callback<void(v8::Local<v8::Value>,
                              v8::Local<v8::Value>)> MessageCallback;

  bool HookWindowMessage(UINT message, const MessageCallback& callback);
  bool IsWindowMessageHooked(UINT message);
  void UnhookWindowMessage(UINT message);
  void UnhookAllWindowMessages();
#endif

#if defined(OS_MACOSX)
  void ShowDefinitionForSelection();
#endif

#if defined(TOOLKIT_VIEWS)
  void SetIcon(mate::Handle<NativeImage> icon);
#endif

  void SetVisibleOnAllWorkspaces(bool visible);
  bool IsVisibleOnAllWorkspaces();

  int32_t ID() const;
  v8::Local<v8::Value> WebContents(v8::Isolate* isolate);

#if defined(OS_WIN)
  typedef std::map<UINT, MessageCallback> MessageCallbackMap;
  MessageCallbackMap messages_callback_map_;
#endif

  v8::Global<v8::Value> web_contents_;
  v8::Global<v8::Value> menu_;

  api::WebContents* api_web_contents_;

  scoped_ptr<NativeWindow> window_;

  DISALLOW_COPY_AND_ASSIGN(Window);
};

}  // namespace api

}  // namespace atom


namespace mate {

template<>
struct Converter<atom::NativeWindow*> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     atom::NativeWindow** out) {
    // null would be tranfered to NULL.
    if (val->IsNull()) {
      *out = NULL;
      return true;
    }

    atom::api::Window* window;
    if (!Converter<atom::api::Window*>::FromV8(isolate, val, &window))
      return false;
    *out = window->window();
    return true;
  }
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_ATOM_API_WINDOW_H_
