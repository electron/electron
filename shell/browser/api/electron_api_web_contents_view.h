// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_

#include <optional>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "shell/browser/api/electron_api_view.h"
#include "shell/browser/draggable_region_provider.h"
#include "shell/browser/native_window_observer.h"

namespace gin_helper {
class Dictionary;
}

namespace electron {
class NativeWindow;
}

namespace electron::api {

class WebContents;

class WebContentsView : public View,
                        private content::WebContentsObserver,
                        private NativeWindowObserver,
                        public DraggableRegionProvider {
 public:
  // Create a new instance of WebContentsView.
  static gin_helper::Handle<WebContentsView> Create(
      v8::Isolate* isolate,
      const gin_helper::Dictionary& web_preferences);

  // Return the cached constructor function.
  static v8::Local<v8::Function> GetConstructor(v8::Isolate* isolate);

  // gin_helper::Wrappable
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Public APIs.
  gin_helper::Handle<WebContents> GetWebContents(v8::Isolate* isolate);
  void SetBackgroundColor(std::optional<WrappedSkColor> color);
  void SetBorderRadius(int radius);
  void SetInteractive(bool interactive) override;

  int NonClientHitTest(const gfx::Point& point) override;

 protected:
  // Takes an existing WebContents.
  WebContentsView(v8::Isolate* isolate,
                  gin_helper::Handle<WebContents> web_contents);
  ~WebContentsView() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;

  // views::ViewObserver
  void OnViewAddedToWidget(views::View* view) override;
  void OnViewRemovedFromWidget(views::View* view) override;

  // NativeWindowObserver
  void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) override;

 private:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  void ApplyBorderRadius();
  void ApplyInteractive();
  void StopObservingWindow();
  void OnContentsBoundsChanging();
  bool HasLivePage();
  void ScheduleWindowControlsOverlayUpdate();
  void SendWindowControlsOverlay();

  // Keep a reference to v8 wrapper.
  v8::Global<v8::Value> web_contents_;
  base::WeakPtr<api::WebContents> api_web_contents_;
  base::WeakPtr<NativeWindow> observed_window_;
  bool window_controls_overlay_update_pending_ = false;

  base::WeakPtrFactory<WebContentsView> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_
