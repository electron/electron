// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_BROWSER_WINDOW_H_
#define SHELL_BROWSER_API_ELECTRON_API_BROWSER_WINDOW_H_

#include <memory>
#include <string>
#include <vector>

#include "base/cancelable_callback.h"
#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/common/gin_helper/error_thrower.h"

namespace electron {

namespace api {

class BrowserWindow : public BaseWindow,
                      public content::RenderWidgetHost::InputEventObserver,
                      public content::WebContentsObserver,
                      public ExtendedWebContentsObserver {
 public:
  static gin_helper::WrappableBase* New(gin_helper::ErrorThrower thrower,
                                        gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Returns the BrowserWindow object from |native_window|.
  static v8::Local<v8::Value> From(v8::Isolate* isolate,
                                   NativeWindow* native_window);

  base::WeakPtr<BrowserWindow> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 protected:
  BrowserWindow(gin::Arguments* args, const gin_helper::Dictionary& options);
  ~BrowserWindow() override;

  // content::RenderWidgetHost::InputEventObserver:
  void OnInputEvent(const blink::WebInputEvent& event) override;

  // content::WebContentsObserver:
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void DidFirstVisuallyNonEmptyPaint() override;
  void BeforeUnloadDialogCancelled() override;
  void OnRendererUnresponsive(content::RenderProcessHost*) override;
  void OnRendererResponsive(
      content::RenderProcessHost* render_process_host) override;

  // ExtendedWebContentsObserver:
  void OnCloseContents() override;
  void OnDraggableRegionsUpdated(
      const std::vector<mojom::DraggableRegionPtr>& regions) override;
  void OnSetContentBounds(const gfx::Rect& rect) override;
  void OnActivateContents() override;
  void OnPageTitleUpdated(const base::string16& title,
                          bool explicit_set) override;

  // NativeWindowObserver:
  void RequestPreferredWidth(int* width) override;
  void OnCloseButtonClicked(bool* prevent_default) override;
  void OnWindowIsKeyChanged(bool is_key) override;

  // BaseWindow:
  void OnWindowClosed() override;
  void OnWindowBlur() override;
  void OnWindowFocus() override;
  void OnWindowResize() override;
  void OnWindowLeaveFullScreen() override;
  void Focus() override;
  void Blur() override;
  void SetBackgroundColor(const std::string& color_name) override;
  void SetBrowserView(v8::Local<v8::Value> value) override;
  void AddBrowserView(v8::Local<v8::Value> value) override;
  void RemoveBrowserView(v8::Local<v8::Value> value) override;
  void ResetBrowserViews() override;
  void SetVibrancy(v8::Isolate* isolate, v8::Local<v8::Value> value) override;
  void OnWindowShow() override;
  void OnWindowHide() override;

  // BrowserWindow APIs.
  void FocusOnWebView();
  void BlurWebView();
  bool IsWebViewFocused();
  v8::Local<v8::Value> GetWebContents(v8::Isolate* isolate);

 private:
#if defined(OS_MACOSX)
  void OverrideNSWindowContentView(InspectableWebContents* iwc);
#endif

  // Helpers.

  // Called when the window needs to update its draggable region.
  void UpdateDraggableRegions(
      const std::vector<mojom::DraggableRegionPtr>& regions);

  // Convert draggable regions in raw format to SkRegion format.
  std::unique_ptr<SkRegion> DraggableRegionsToSkRegion(
      const std::vector<mojom::DraggableRegionPtr>& regions);

  // Schedule a notification unresponsive event.
  void ScheduleUnresponsiveEvent(int ms);

  // Dispatch unresponsive event to observers.
  void NotifyWindowUnresponsive();

  // Cleanup our WebContents observers.
  void Cleanup();

  // Closure that would be called when window is unresponsive when closing,
  // it should be cancelled when we can prove that the window is responsive.
  base::CancelableClosure window_unresponsive_closure_;

#if defined(OS_MACOSX)
  std::vector<mojom::DraggableRegionPtr> draggable_regions_;
#endif

  v8::Global<v8::Value> web_contents_;
  base::WeakPtr<api::WebContents> api_web_contents_;

  base::WeakPtrFactory<BrowserWindow> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BrowserWindow);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_BROWSER_WINDOW_H_
