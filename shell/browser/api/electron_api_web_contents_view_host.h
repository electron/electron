// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_HOST_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_HOST_H_

#include <optional>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "shell/browser/api/electron_api_view.h"
#include "shell/browser/draggable_region_provider.h"
#include "shell/browser/native_window_observer.h"
#include "v8/include/cppgc/persistent.h"

namespace gfx {
class Point;
class Rect;
}  // namespace gfx

namespace views {
class View;
}  // namespace views

namespace electron {
class NativeWindow;
}  // namespace electron

namespace electron::api {

class WebContents;
class WebContentsView;

class WebContentsViewHost final : public View::Host,
                                  public content::WebContentsObserver,
                                  public NativeWindowObserver,
                                  public DraggableRegionProvider {
 public:
  WebContentsViewHost(WebContentsView* wrapper, WebContents* web_contents);

  void RegisterDraggableRegionProvider(NativeWindow* window);
  void ApplyBorderRadius(std::optional<int> radius);
  void ApplyInteractive(bool interactive);

  // DraggableRegionProvider:
  int NonClientHitTest(const gfx::Point& point) override;

 private:
  ~WebContentsViewHost() override;

  // The wrapper of a WebContentsViewHost is always a WebContentsView.
  static WebContentsView* AsWebContentsView(const WrapperRef<View>& api_view) {
    return static_cast<WebContentsView*>(api_view.operator->());
  }

  // NativePeer:
  void OnShutdown() override;
  void TearDownNative() override;

  // views::ViewObserver:
  void OnViewAddedToWidget(views::View* observed_view) override;
  void OnViewRemovedFromWidget(views::View* observed_view) override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  // NativeWindowObserver:
  void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) override;

  WebContents* GetLiveWebContents() const;
  bool IsViewOwner() const;
  void StopObservingWindow();
  void UnregisterDraggableRegionProvider();
  void OnContentsBoundsChanging();
  bool HasLivePage();
  void ScheduleWindowControlsOverlayUpdate();
  void SendWindowControlsOverlay();

  // The wrapper holds the strong edge. This one lets the host destroy
  // the WebContents after the wrapper has been collected.
  cppgc::WeakPersistent<WebContents> api_web_contents_;
  base::WeakPtr<NativeWindow> observed_window_;
  base::WeakPtr<NativeWindow> draggable_region_window_;
  bool window_controls_overlay_update_pending_ = false;
  // Cleared at shutdown, where the WebContents disposes itself.
  bool destroy_web_contents_on_release_ = true;

  base::WeakPtrFactory<WebContentsViewHost> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_HOST_H_
