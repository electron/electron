// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_HOST_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_HOST_H_

#include <optional>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "content/public/browser/web_contents_observer.h"
#include "shell/browser/api/electron_api_view.h"
#include "shell/browser/draggable_region_provider.h"
#include "shell/browser/native_window_observer.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "ui/views/view_tracker.h"
#include "v8/include/cppgc/persistent.h"

namespace gfx {
class Point;
class Rect;
}  // namespace gfx

namespace electron {

class InspectableWebContentsView;
class NativeWindow;

}  // namespace electron

namespace electron::api {

class WebContents;
class WebContentsView;

// The native view of a WebContentsView. It holds the webContents'
// InspectableWebContentsView, which the webContents owns and deletes with
// itself, so that the WebContentsView keeps one native view, and its place in
// the view tree, for its whole life. The inspectable view fills it and stays
// its last child. Child views added from JavaScript go before it, where they
// went when they were added to the inspectable view itself.
class WebContentsContainerView : public views::View,
                                 public views::ViewObserver {
  METADATA_HEADER(WebContentsContainerView, views::View)

 public:
  WebContentsContainerView();
  ~WebContentsContainerView() override;

  void TakeInspectableView(InspectableWebContentsView* inspectable_view);

  // The inspectable view, while it is still here. Null once another
  // WebContentsView has adopted the webContents, or the webContents has
  // deleted it.
  InspectableWebContentsView* GetOwnedInspectableView();

  // Whether |child| is the inspectable view and was removed because the
  // webContents is deleting it, rather than because another WebContentsView
  // adopted the webContents.
  bool IsInspectableViewBeingDeleted(const views::View* child) const;

  // Invoked when this view's bounds have changed, including a move that keeps
  // its size, but before its children (and therefore the page's
  // RenderWidgetHostView) have been laid out to match.
  void SetBoundsChangedCallback(base::RepeatingClosure callback);

  // views::View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void Layout(PassKey) override;

 private:
  // views::ViewObserver:
  void OnViewHierarchyWillBeDeleted(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;

  views::ViewTracker inspectable_view_;
  bool inspectable_view_deleting_ = false;
  base::RepeatingClosure bounds_changed_callback_;
  base::ScopedObservation<views::View, views::ViewObserver>
      inspectable_view_observation_{this};
};

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
  void OnChildViewRemoved(views::View* observed_view,
                          views::View* child) override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  // NativeWindowObserver:
  void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) override;

  WebContents* GetLiveWebContents() const;
  WebContentsContainerView* container() const;
  InspectableWebContentsView* GetOwnedInspectableView() const;
  void RemoveFromParent();
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
