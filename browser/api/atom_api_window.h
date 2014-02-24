// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WINDOW_H_
#define ATOM_BROWSER_API_ATOM_API_WINDOW_H_

#include <vector>

#include "base/memory/scoped_ptr.h"
#include "browser/native_window_observer.h"
#include "common/api/atom_api_event_emitter.h"
#include "common/v8/scoped_persistent.h"

namespace base {
class DictionaryValue;
}

namespace atom {

class NativeWindow;

namespace api {

class Window : public EventEmitter,
               public NativeWindowObserver {
 public:
  virtual ~Window();

  static void Initialize(v8::Handle<v8::Object> target);

  NativeWindow* window() { return window_.get(); }

 protected:
  explicit Window(v8::Handle<v8::Object> wrapper,
                  base::DictionaryValue* options);

  // Implementations of NativeWindowObserver.
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
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Destroy(const v8::FunctionCallbackInfo<v8::Value>& args);

  // APIs for NativeWindow.
  static void Close(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Focus(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsFocused(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Show(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Hide(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsVisible(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Maximize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Unmaximize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Minimize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Restore(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetFullscreen(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsFullscreen(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetSize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetSize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetMinimumSize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetMinimumSize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetMaximumSize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetMaximumSize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetResizable(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsResizable(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetAlwaysOnTop(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsAlwaysOnTop(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Center(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetPosition(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetPosition(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetTitle(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetTitle(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void FlashFrame(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetKiosk(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsKiosk(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void OpenDevTools(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CloseDevTools(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsDevToolsOpened(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void InspectElement(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DebugDevTools(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void FocusOnWebView(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void BlurWebView(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsWebViewFocused(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CapturePage(const v8::FunctionCallbackInfo<v8::Value>& args);

  // APIs for WebContents.
  static void GetPageTitle(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsLoading(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsWaitingForResponse(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Stop(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetRoutingID(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetProcessID(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsCrashed(const v8::FunctionCallbackInfo<v8::Value>& args);

  // APIs for NavigationController.
  static void LoadURL(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetURL(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CanGoBack(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CanGoForward(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CanGoToOffset(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GoBack(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GoForward(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GoToIndex(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GoToOffset(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Reload(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ReloadIgnoringCache(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  // Called when capturePage is done.
  void OnCapturePageDone(const RefCountedV8Function& callback,
                         const std::vector<unsigned char>& data);

  scoped_ptr<NativeWindow> window_;

  DISALLOW_COPY_AND_ASSIGN(Window);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WINDOW_H_
