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

// Debugging aid enabled by ELECTRON_DEBUG_DRAGGABLE_REGIONS in unpackaged
// apps: paints the region a WebContents hit tests against in a click-through
// widget above the web contents and logs region updates and hit tests.
class DraggableRegionDebugger : private views::ViewObserver,
                                private views::WidgetObserver {
 public:
  static bool IsEnabled();

  // |contents_view| may be null, in which case only logging is performed.
  DraggableRegionDebugger(int32_t web_contents_id, views::View* contents_view);
  ~DraggableRegionDebugger() override;

  // disable copy
  DraggableRegionDebugger(const DraggableRegionDebugger&) = delete;
  DraggableRegionDebugger& operator=(const DraggableRegionDebugger&) = delete;

  // |region| is null when the update was ignored because the window has a
  // native frame.
  void OnRegionsChanged(
      const std::vector<blink::mojom::DraggableRegionPtr>& regions,
      const SkRegion* region,
      base::TimeDelta compute_time);

  void OnHitTest(base::TimeDelta duration, bool hit);

 private:
  class OverlayView;

  void EnsureOverlay();
  void DestroyOverlay();
  void UpdateOverlay();

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

  // views::WidgetObserver:
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

  SkRegion region_;

  std::unique_ptr<views::Widget> overlay_widget_;
  raw_ptr<OverlayView> overlay_view_ = nullptr;

  uint64_t update_count_ = 0;
  base::TimeTicks last_update_time_;

  gfx::Rect last_bounds_;
  base::TimeTicks last_bounds_change_time_;

  uint64_t hit_test_count_ = 0;
  uint64_t hit_test_hits_ = 0;
  base::TimeDelta hit_test_total_;
  base::TimeDelta hit_test_max_;
  base::TimeTicks hit_test_window_start_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_DRAGGABLE_REGION_DEBUGGER_H_
