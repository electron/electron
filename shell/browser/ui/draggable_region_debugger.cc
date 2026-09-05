// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/draggable_region_debugger.h"

#include <algorithm>
#include <cinttypes>
#include <string>
#include <utility>

#include "base/environment.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "shell/browser/api/electron_api_app.h"
#include "third_party/blink/public/mojom/page/draggable_region.mojom.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/skia_conversions.h"

namespace electron {

namespace {

constexpr char kEnvVar[] = "ELECTRON_DEBUG_DRAGGABLE_REGIONS";

// Every log line from this file carries this prefix so it can be grepped out
// of the rest of the Chromium log.
constexpr char kLogPrefix[] = "[draggable-regions]";

// Hit test summaries are emitted at most this often.
constexpr base::TimeDelta kHitTestLogInterval = base::Seconds(2);

constexpr SkColor kRegionFillColor = SkColorSetARGB(0x50, 0xE5, 0x39, 0x35);
constexpr SkColor kRegionStrokeColor = SkColorSetARGB(0xC8, 0xE5, 0x39, 0x35);
// Parts of the region that differ from the previous update are tinted so a
// change is visible even when the rectangles barely move.
constexpr SkColor kChangedFillColor = SkColorSetARGB(0x70, 0xFF, 0xC1, 0x07);
constexpr SkColor kStampBackground = SkColorSetARGB(0xB0, 0x00, 0x00, 0x00);
constexpr SkColor kStampText = SK_ColorWHITE;

std::string FormatMillis(base::TimeDelta delta) {
  return base::StringPrintf("%.1f ms", delta.InMillisecondsF());
}

std::string FormatMicros(base::TimeDelta delta) {
  return base::StringPrintf("%.1f us", delta.InMicrosecondsF());
}

}  // namespace

// Paints the rectangles that make up the draggable region. Lives in the
// overlay widget, whose bounds match the contents view, so the region's
// coordinates can be used as-is.
class DraggableRegionDebugger::OverlayView : public views::View {
 public:
  OverlayView() = default;
  ~OverlayView() override = default;

  void SetRegion(const SkRegion& region,
                 uint64_t update_number,
                 base::TimeTicks update_time) {
    changed_.op(region_, region, SkRegion::kXOR_Op);
    region_ = region;
    update_number_ = update_number;
    update_time_ = update_time;
    SchedulePaint();
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    TRACE_EVENT1("electron", "DraggableRegionDebugger::OverlayView::OnPaint",
                 "update", update_number_);
    for (SkRegion::Iterator it(region_); !it.done(); it.next()) {
      const gfx::Rect rect = gfx::SkIRectToRect(it.rect());
      canvas->FillRect(rect, kRegionFillColor);
      canvas->DrawRect(gfx::RectF(rect), kRegionStrokeColor);
    }
    for (SkRegion::Iterator it(changed_); !it.done(); it.next())
      canvas->FillRect(gfx::SkIRectToRect(it.rect()), kChangedFillColor);

    // Stamp the overlay with which update it shows and how old that update
    // was when this paint ran, so a stale overlay can be told apart from
    // stale region data.
    const std::string stamp = base::StringPrintf(
        "draggable regions: update #%" PRIu64 ", painted %s after it arrived",
        update_number_,
        FormatMillis(base::TimeTicks::Now() - update_time_).c_str());
    const gfx::Rect stamp_rect(4, 4, 360, 18);
    canvas->FillRect(stamp_rect, kStampBackground);
    canvas->DrawStringRect(base::UTF8ToUTF16(stamp), gfx::FontList(),
                           kStampText, stamp_rect);
  }

 private:
  SkRegion region_;
  SkRegion changed_;
  uint64_t update_number_ = 0;
  base::TimeTicks update_time_;
};

// static
bool DraggableRegionDebugger::IsEnabled() {
  static const bool enabled =
      base::Environment::Create()->HasVar(kEnvVar) && !api::App::IsPackaged();
  return enabled;
}

DraggableRegionDebugger::DraggableRegionDebugger(int32_t web_contents_id,
                                                 views::View* contents_view)
    : web_contents_id_(web_contents_id), contents_view_(contents_view) {
  ObserveViewAncestry();

  LOG(INFO) << kLogPrefix << " webContents " << web_contents_id_
            << ": debugging enabled"
            << (contents_view_ ? "" : " (no view to draw the overlay over)");
}

DraggableRegionDebugger::~DraggableRegionDebugger() {
  DestroyOverlay();
}

void DraggableRegionDebugger::OnRegionsChanged(
    const std::vector<blink::mojom::DraggableRegionPtr>& regions,
    const SkRegion* region,
    base::TimeDelta compute_time) {
  const auto draggable_count = static_cast<size_t>(std::ranges::count_if(
      regions, [](const auto& entry) { return entry->draggable; }));
  const size_t no_drag_count = regions.size() - draggable_count;

  if (!region) {
    LOG(INFO) << kLogPrefix << " webContents " << web_contents_id_
              << ": ignored " << regions.size() << " region(s) ("
              << draggable_count << " drag, " << no_drag_count
              << " no-drag) because the window has a native frame";
    return;
  }

  const base::TimeTicks now = base::TimeTicks::Now();
  ++update_count_;
  TRACE_EVENT1("electron", "DraggableRegionDebugger::OnRegionsChanged",
               "update", update_count_);

  std::string timing;
  if (!last_update_time_.is_null()) {
    timing +=
        ", " + FormatMillis(now - last_update_time_) + " since previous update";
  }
  // How long the browser hit tested against the previous region after the
  // contents view last changed size. During a resize this is the window in
  // which clicks land on stale rectangles.
  if (!last_bounds_change_time_.is_null()) {
    timing += ", " + FormatMillis(now - last_bounds_change_time_) +
              " after last bounds change to " + last_bounds_.size().ToString();
  }
  last_update_time_ = now;

  const bool changed = region_ != *region;
  LOG(INFO) << kLogPrefix << " webContents " << web_contents_id_ << ": update #"
            << update_count_ << timing << ": renderer sent " << regions.size()
            << " region(s) (" << draggable_count << " drag, " << no_drag_count
            << " no-drag); hit-test region computed in "
            << FormatMicros(compute_time) << ": "
            << region->computeRegionComplexity() << " rect(s), bounds "
            << gfx::SkIRectToRect(region->getBounds()).ToString()
            << (changed ? "" : " (identical to previous region)");

  region_ = *region;
  EnsureOverlay();
  if (overlay_view_)
    overlay_view_->SetRegion(region_, update_count_, now);
  UpdateOverlay();
}

void DraggableRegionDebugger::OnHitTest(base::TimeDelta duration, bool hit) {
  const base::TimeTicks now = base::TimeTicks::Now();
  if (hit_test_window_start_.is_null())
    hit_test_window_start_ = now;

  ++hit_test_count_;
  if (hit)
    ++hit_test_hits_;
  hit_test_total_ += duration;
  hit_test_max_ = std::max(hit_test_max_, duration);

  const base::TimeDelta window = now - hit_test_window_start_;
  if (window < kHitTestLogInterval)
    return;

  LOG(INFO) << kLogPrefix << " webContents " << web_contents_id_ << ": "
            << hit_test_count_ << " hit test(s) in the last "
            << base::StringPrintf("%.1f s", window.InSecondsF()) << " ("
            << hit_test_hits_ << " inside a draggable region), total "
            << FormatMicros(hit_test_total_) << ", avg "
            << FormatMicros(hit_test_total_ / hit_test_count_) << ", max "
            << FormatMicros(hit_test_max_);

  hit_test_window_start_ = now;
  hit_test_count_ = 0;
  hit_test_hits_ = 0;
  hit_test_total_ = base::TimeDelta();
  hit_test_max_ = base::TimeDelta();
}

void DraggableRegionDebugger::EnsureOverlay() {
  if (overlay_widget_ || !contents_view_)
    return;

  views::Widget* parent_widget = contents_view_->GetWidget();
  if (!parent_widget)
    return;

  overlay_widget_ = std::make_unique<views::Widget>();
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_POPUP);
  params.name = "DraggableRegionDebugOverlay";
  params.parent = parent_widget->GetNativeView();
  params.context = parent_widget->GetNativeWindow();
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.shadow_type = views::Widget::InitParams::ShadowType::kNone;
  params.activatable = views::Widget::InitParams::Activatable::kNo;
  // The overlay must never get in the way of the regions it visualizes.
  params.accept_events = false;
  overlay_widget_->Init(std::move(params));
  overlay_widget_->SetVisibilityChangedAnimationsEnabled(false);
  overlay_view_ =
      overlay_widget_->SetContentsView(std::make_unique<OverlayView>());
  overlay_view_->SetRegion(region_, update_count_, last_update_time_);

  widget_observation_.Reset();
  widget_observation_.Observe(parent_widget);
}

void DraggableRegionDebugger::DestroyOverlay() {
  widget_observation_.Reset();
  overlay_view_ = nullptr;
  overlay_widget_.reset();
}

void DraggableRegionDebugger::UpdateOverlay() {
  if (!overlay_widget_)
    return;

  views::Widget* parent_widget =
      contents_view_ ? contents_view_->GetWidget() : nullptr;
  const gfx::Rect bounds =
      contents_view_ ? contents_view_->GetBoundsInScreen() : gfx::Rect();
  const bool show = parent_widget && parent_widget->IsVisible() &&
                    contents_view_->IsDrawn() && !bounds.IsEmpty() &&
                    !region_.isEmpty();
  if (!show) {
    if (overlay_widget_->IsVisible())
      overlay_widget_->Hide();
    return;
  }

  if (bounds != last_bounds_) {
    const base::TimeTicks now = base::TimeTicks::Now();
    TRACE_EVENT0("electron", "DraggableRegionDebugger::BoundsChanged");
    LOG(INFO) << kLogPrefix << " webContents " << web_contents_id_
              << ": contents view bounds changed to " << bounds.ToString()
              << (last_bounds_change_time_.is_null()
                      ? ""
                      : ", " + FormatMillis(now - last_bounds_change_time_) +
                            " since previous bounds change")
              << "; overlay still shows update #" << update_count_;
    last_bounds_ = bounds;
    last_bounds_change_time_ = now;
    overlay_widget_->SetBounds(bounds);
  }
  if (!overlay_widget_->IsVisible())
    overlay_widget_->ShowInactive();
}

void DraggableRegionDebugger::ObserveViewAncestry() {
  view_observations_.RemoveAllObservations();
  for (views::View* view = contents_view_; view; view = view->parent())
    view_observations_.AddObservation(view);
}

void DraggableRegionDebugger::OnViewBoundsChanged(views::View* observed_view) {
  UpdateOverlay();
}

void DraggableRegionDebugger::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view,
    bool visible) {
  UpdateOverlay();
}

void DraggableRegionDebugger::OnViewHierarchyChanged(
    views::View* observed_view,
    const views::ViewHierarchyChangedDetails& details) {
  ObserveViewAncestry();
  UpdateOverlay();
}

void DraggableRegionDebugger::OnViewAddedToWidget(views::View* observed_view) {
  if (observed_view != contents_view_)
    return;
  ObserveViewAncestry();
  EnsureOverlay();
  UpdateOverlay();
}

void DraggableRegionDebugger::OnViewRemovedFromWidget(
    views::View* observed_view) {
  if (observed_view != contents_view_)
    return;
  // The overlay is parented to the widget the view just left.
  DestroyOverlay();
  ObserveViewAncestry();
}

void DraggableRegionDebugger::OnViewIsDeleting(views::View* observed_view) {
  if (observed_view != contents_view_) {
    view_observations_.RemoveObservation(observed_view);
    return;
  }
  DestroyOverlay();
  view_observations_.RemoveAllObservations();
  contents_view_ = nullptr;
}

void DraggableRegionDebugger::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  UpdateOverlay();
}

void DraggableRegionDebugger::OnWidgetVisibilityChanged(views::Widget* widget,
                                                        bool visible) {
  UpdateOverlay();
}

void DraggableRegionDebugger::OnWidgetDestroying(views::Widget* widget) {
  DestroyOverlay();
}

}  // namespace electron
