// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WINDOW_H_
#define ATOM_BROWSER_API_ATOM_API_WINDOW_H_

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "atom/browser/native_window_observer.h"
#include "atom/browser/api/event_emitter.h"
#include "native_mate/handle.h"

class GURL;

namespace mate {
class Arguments;
class Dictionary;
}

namespace atom {

class NativeWindow;

namespace api {

class WebContents;

class Window : public mate::EventEmitter,
               public NativeWindowObserver {
 public:
  static mate::Wrappable* New(const mate::Dictionary& options);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Handle<v8::ObjectTemplate> prototype);

  NativeWindow* window() const { return window_.get(); }

 protected:
  explicit Window(const mate::Dictionary& options);
  virtual ~Window();

  // Implementations of NativeWindowObserver:
  virtual void OnPageTitleUpdated(bool* prevent_default,
                                  const std::string& title) OVERRIDE;
  virtual void WillCloseWindow(bool* prevent_default) OVERRIDE;
  virtual void OnWindowClosed() OVERRIDE;
  virtual void OnWindowBlur() OVERRIDE;
  virtual void OnWindowFocus() OVERRIDE;
  virtual void OnRendererUnresponsive() OVERRIDE;
  virtual void OnRendererResponsive() OVERRIDE;

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
  bool IsMaximized();
  void Minimize();
  void Restore();
  void SetFullscreen(bool fullscreen);
  bool IsFullscreen();
  void SetSize(int width, int height);
  std::vector<int> GetSize();
  void SetContentSize(int width, int height);
  std::vector<int> GetContentSize();
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
  void SetSkipTaskbar(bool skip);
  void SetKiosk(bool kiosk);
  bool IsKiosk();
  void OpenDevTools();
  void CloseDevTools();
  bool IsDevToolsOpened();
  void InspectElement(int x, int y);
  void FocusOnWebView();
  void BlurWebView();
  bool IsWebViewFocused();
  void CapturePage(mate::Arguments* args);
  void SetRepresentedFilename(const std::string& filename);
  void SetDocumentEdited(bool edited);

  // APIs for WebContents.
  mate::Handle<WebContents> GetWebContents(v8::Isolate* isolate) const;
  mate::Handle<WebContents> GetDevToolsWebContents(v8::Isolate* isolate) const;

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
