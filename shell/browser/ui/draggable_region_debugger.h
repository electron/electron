// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_DRAGGABLE_REGION_DEBUGGER_H_
#define ELECTRON_SHELL_BROWSER_UI_DRAGGABLE_REGION_DEBUGGER_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "third_party/blink/public/mojom/page/draggable_region.mojom-forward.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace electron {

// Debugging aid for draggable regions. Enabled by setting the
// ELECTRON_DEBUG_DRAGGABLE_REGIONS environment variable in an unpackaged app.
//
// Paints the region a WebContents hit tests against (the union of every
// `app-region: drag` rectangle minus every `app-region: no-drag` rectangle,
// as received from the renderer) as translucent rectangles in a click-through
// widget floating above the web contents, and logs the cost of each region
// update along with a periodic summary of the hit tests it serves.
class DraggableRegionDebugger : private views::ViewObserver,
                                private views::WidgetObserver {
 public:
  static bool IsEnabled();

  // |web_contents_id| labels the log lines. |contents_view| is the view that
  // hosts the web contents; the overlay follows its bounds. It may be null
  // when there is nothing on screen to paint over, in which case only logging
  // is performed.
  DraggableRegionDebugger(int32_t web_contents_id, views::View* contents_view);
  ~DraggableRegionDebugger() override;

  // disable copy
  DraggableRegionDebugger(const DraggableRegionDebugger&) = delete;
  DraggableRegionDebugger& operator=(const DraggableRegionDebugger&) = delete;

  // Called with every set of regions the renderer sends. |region| is the
  // hit-test region computed from them and |compute_time| how long that took,
  // or null when the update was ignored because the window has a native frame.
  void OnRegionsChanged(
      const std::vector<blink::mojom::DraggableRegionPtr>& regions,
      const SkRegion* region,
      base::TimeDelta compute_time);

  // Called with the outcome and duration of every hit test against the region.
  void OnHitTest(base::TimeDelta duration, bool hit);

 private:
  class OverlayView;

  void EnsureOverlay();
  void DestroyOverlay();
  void UpdateOverlay();

  // Observes |contents_view_| and every ancestor, so the overlay follows the
  // web contents when any view above it moves as well.
  void ObserveViewAncestry();

  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view,
                               bool visible) override;
  void OnViewHierarchyChanged(
      views::View* observed_view,
      const views::ViewHierarchyChangedDetails& details) override;
  void OnViewAddedToWidget(views::View* observed_view) override;
  void OnViewRemovedFromWidget(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;

  // views::WidgetObserver (observes the widget hosting |contents_view_|):
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  const int32_t web_contents_id_;

  raw_ptr<views::View> contents_view_;
  base::ScopedMultiSourceObservation<views::View, views::ViewObserver>
      view_observations_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};

  // The most recently computed region, kept so the overlay can be rebuilt
  // when the web contents moves to another widget.
  SkRegion region_;

  std::unique_ptr<views::Widget> overlay_widget_;
  raw_ptr<OverlayView> overlay_view_ = nullptr;

  // Region update statistics.
  uint64_t update_count_ = 0;
  base::TimeTicks last_update_time_;

  // The last contents view bounds the overlay was positioned over, and when
  // they changed. Region updates are logged relative to this so the window of
  // hit testing against stale regions after a resize is visible.
  gfx::Rect last_bounds_;
  base::TimeTicks last_bounds_change_time_;

  // Hit test statistics, accumulated between periodic summary log lines.
  uint64_t hit_test_count_ = 0;
  uint64_t hit_test_hits_ = 0;
  base::TimeDelta hit_test_total_;
  base::TimeDelta hit_test_max_;
  base::TimeTicks hit_test_window_start_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_DRAGGABLE_REGION_DEBUGGER_H_
