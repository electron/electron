// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WINDOW_H_
#define ATOM_BROWSER_API_ATOM_API_WINDOW_H_

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "atom/browser/native_window_observer.h"
#include "atom/browser/api/event_emitter.h"

class GURL;

namespace base {
class DictionaryValue;
}

namespace mate {
class Arguments;
class Dictionary;
}

namespace atom {

class NativeWindow;

namespace api {

class Window : public mate::EventEmitter,
               public NativeWindowObserver {
 public:
  static mate::Wrappable* New(mate::Arguments* args,
                              const base::DictionaryValue& options);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Handle<v8::ObjectTemplate> prototype);

  NativeWindow* window() const { return window_.get(); }

 protected:
  explicit Window(base::DictionaryValue* options);
  virtual ~Window();

  // Implementations of NativeWindowObserver:
  virtual void OnPageTitleUpdated(bool* prevent_default,
                                  const std::string& title) OVERRIDE;
  virtual void OnLoadingStateChanged(bool is_loading) OVERRIDE;
  virtual void WillCloseWindow(bool* prevent_default) OVERRIDE;
  virtual void OnWindowClosed() OVERRIDE;
  virtual void OnWindowBlur() OVERRIDE;
  virtual void OnRendererUnresponsive() OVERRIDE;
  virtual void OnRendererResponsive() OVERRIDE;
  virtual void OnRenderViewDeleted(int process_id, int routing_id) OVERRIDE;
  virtual void OnRendererCrashed() OVERRIDE;

 private:
  // APIs for NativeWindow.
  void Destroy();
  void Close();
  void Focus();
  bool IsFocused();
  void Show();
  void Hide();
  bool IsVisible();
  void Maximize();
  void Unmaximize();
  void Minimize();
  void Restore();
  void SetFullscreen(bool fullscreen);
  bool IsFullscreen();
  void SetSize(int width, int height);
  std::vector<int> GetSize();
  void SetMinimumSize(int width, int height);
  std::vector<int> GetMinimumSize();
  void SetMaximumSize(int width, int height);
  std::vector<int> GetMaximumSize();
  void SetResizable(bool resizable);
  bool IsResizable();
  void SetAlwaysOnTop(bool top);
  bool IsAlwaysOnTop();
  void Center();
  void SetPosition(int x, int y);
  std::vector<int> GetPosition();
  void SetTitle(const std::string& title);
  std::string GetTitle();
  void FlashFrame(bool flash);
  void SetKiosk(bool kiosk);
  bool IsKiosk();
  void OpenDevTools();
  void CloseDevTools();
  bool IsDevToolsOpened();
  void InspectElement(int x, int y);
  void DebugDevTools();
  void FocusOnWebView();
  void BlurWebView();
  bool IsWebViewFocused();
  void CapturePage(mate::Arguments* args);

  // APIs for WebContents.
  string16 GetPageTitle();
  bool IsLoading();
  bool IsWaitingForResponse();
  void Stop();
  int GetRoutingID();
  int GetProcessID();
  bool IsCrashed();

  // APIs for devtools.
  mate::Dictionary GetDevTools(v8::Isolate* isolate);
  void ExecuteJavaScriptInDevTools(const std::string& code);

  // APIs for NavigationController.
  void LoadURL(const GURL& url);
  GURL GetURL();
  bool CanGoBack();
  bool CanGoForward();
  bool CanGoToOffset(int offset);
  void GoBack();
  void GoForward();
  void GoToIndex(int index);
  void GoToOffset(int offset);
  void Reload();
  void ReloadIgnoringCache();

  scoped_ptr<NativeWindow> window_;

  DISALLOW_COPY_AND_ASSIGN(Window);
};

}  // namespace api

}  // namespace atom


namespace mate {

template<>
struct Converter<atom::NativeWindow*> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
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
