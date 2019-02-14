// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_render_widget_host_view.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "components/viz/common/features.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/delay_based_time_source.h"
#include "components/viz/common/gl_helper.h"
#include "components/viz/common/quads/render_pass.h"
#include "content/browser/renderer_host/cursor_manager.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_owner_delegate.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/context_factory.h"
#include "content/public/browser/render_process_host.h"
#include "media/base/video_frame.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_type.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/latency/latency_info.h"

namespace atom {

namespace {

const float kDefaultScaleFactor = 1.0;
const int kFrameRetryLimit = 2;

ui::MouseEvent UiMouseEventFromWebMouseEvent(blink::WebMouseEvent event) {
  ui::EventType type = ui::EventType::ET_UNKNOWN;
  switch (event.GetType()) {
    case blink::WebInputEvent::kMouseDown:
      type = ui::EventType::ET_MOUSE_PRESSED;
      break;
    case blink::WebInputEvent::kMouseUp:
      type = ui::EventType::ET_MOUSE_RELEASED;
      break;
    case blink::WebInputEvent::kMouseMove:
      type = ui::EventType::ET_MOUSE_MOVED;
      break;
    case blink::WebInputEvent::kMouseEnter:
      type = ui::EventType::ET_MOUSE_ENTERED;
      break;
    case blink::WebInputEvent::kMouseLeave:
      type = ui::EventType::ET_MOUSE_EXITED;
      break;
    case blink::WebInputEvent::kMouseWheel:
      type = ui::EventType::ET_MOUSEWHEEL;
      break;
    default:
      type = ui::EventType::ET_UNKNOWN;
      break;
  }

  int button_flags = 0;
  switch (event.button) {
    case blink::WebMouseEvent::Button::kBack:
      button_flags |= ui::EventFlags::EF_BACK_MOUSE_BUTTON;
      break;
    case blink::WebMouseEvent::Button::kForward:
      button_flags |= ui::EventFlags::EF_FORWARD_MOUSE_BUTTON;
      break;
    case blink::WebMouseEvent::Button::kLeft:
      button_flags |= ui::EventFlags::EF_LEFT_MOUSE_BUTTON;
      break;
    case blink::WebMouseEvent::Button::kMiddle:
      button_flags |= ui::EventFlags::EF_MIDDLE_MOUSE_BUTTON;
      break;
    case blink::WebMouseEvent::Button::kRight:
      button_flags |= ui::EventFlags::EF_RIGHT_MOUSE_BUTTON;
      break;
    default:
      button_flags = 0;
      break;
  }

  ui::MouseEvent ui_event(type,
                          gfx::Point(std::floor(event.PositionInWidget().x),
                                     std::floor(event.PositionInWidget().y)),
                          gfx::Point(std::floor(event.PositionInWidget().x),
                                     std::floor(event.PositionInWidget().y)),
                          ui::EventTimeForNow(), button_flags, button_flags);
  ui_event.SetClickCount(event.click_count);

  return ui_event;
}

ui::MouseWheelEvent UiMouseWheelEventFromWebMouseEvent(
    blink::WebMouseWheelEvent event) {
  return ui::MouseWheelEvent(UiMouseEventFromWebMouseEvent(event),
                             std::floor(event.delta_x),
                             std::floor(event.delta_y));
}

}  // namespace

class AtomCopyFrameGenerator {
 public:
  AtomCopyFrameGenerator(OffScreenRenderWidgetHostView* view,
                         int frame_rate_threshold_us)
      : view_(view),
        frame_duration_(
            base::TimeDelta::FromMicroseconds(frame_rate_threshold_us)),
        weak_ptr_factory_(this) {
    last_time_ = base::Time::Now();
  }

  void GenerateCopyFrame(const gfx::Rect& damage_rect) {
    if (!view_->render_widget_host() || !view_->IsPainting())
      return;

    auto request = std::make_unique<viz::CopyOutputRequest>(
        viz::CopyOutputRequest::ResultFormat::RGBA_BITMAP,
        base::BindOnce(
            &AtomCopyFrameGenerator::CopyFromCompositingSurfaceHasResult,
            weak_ptr_factory_.GetWeakPtr(), damage_rect));

    request->set_area(gfx::Rect(view_->GetCompositorViewportPixelSize()));
    view_->GetRootLayer()->RequestCopyOfOutput(std::move(request));
  }

  void set_frame_rate_threshold_us(int frame_rate_threshold_us) {
    frame_duration_ =
        base::TimeDelta::FromMicroseconds(frame_rate_threshold_us);
  }

 private:
  void CopyFromCompositingSurfaceHasResult(
      const gfx::Rect& damage_rect,
      std::unique_ptr<viz::CopyOutputResult> result) {
    if (result->IsEmpty() || result->size().IsEmpty() ||
        !view_->render_widget_host()) {
      OnCopyFrameCaptureFailure(damage_rect);
      return;
    }

    DCHECK(!result->IsEmpty());
    auto source = std::make_unique<SkBitmap>(result->AsSkBitmap());
    DCHECK(source->readyToDraw());
    if (source) {
      base::AutoLock autolock(lock_);
      std::shared_ptr<SkBitmap> bitmap(source.release());

      base::TimeTicks now = base::TimeTicks::Now();
      base::TimeDelta next_frame_in = next_frame_time_ - now;
      if (next_frame_in > frame_duration_ / 4) {
        next_frame_time_ += frame_duration_;
        base::PostDelayedTaskWithTraits(
            FROM_HERE, {content::BrowserThread::UI},
            base::BindOnce(&AtomCopyFrameGenerator::OnCopyFrameCaptureSuccess,
                           weak_ptr_factory_.GetWeakPtr(), damage_rect, bitmap),
            next_frame_in);
      } else {
        next_frame_time_ = now + frame_duration_;
        OnCopyFrameCaptureSuccess(damage_rect, bitmap);
      }

      frame_retry_count_ = 0;
    } else {
      OnCopyFrameCaptureFailure(damage_rect);
    }
  }

  void OnCopyFrameCaptureFailure(const gfx::Rect& damage_rect) {
    const bool force_frame = (++frame_retry_count_ <= kFrameRetryLimit);
    if (force_frame) {
      // Retry with the same |damage_rect|.
      base::PostTaskWithTraits(
          FROM_HERE, {content::BrowserThread::UI},
          base::BindOnce(&AtomCopyFrameGenerator::GenerateCopyFrame,
                         weak_ptr_factory_.GetWeakPtr(), damage_rect));
    }
  }

  void OnCopyFrameCaptureSuccess(const gfx::Rect& damage_rect,
                                 const std::shared_ptr<SkBitmap>& bitmap) {
    base::AutoLock lock(onPaintLock_);
    view_->OnPaint(damage_rect, *bitmap);
  }

  base::Lock lock_;
  base::Lock onPaintLock_;
  OffScreenRenderWidgetHostView* view_;

  base::Time last_time_;

  int frame_retry_count_ = 0;
  base::TimeTicks next_frame_time_ = base::TimeTicks::Now();
  base::TimeDelta frame_duration_;

  base::WeakPtrFactory<AtomCopyFrameGenerator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomCopyFrameGenerator);
};

class AtomBeginFrameTimer : public viz::DelayBasedTimeSourceClient {
 public:
  AtomBeginFrameTimer(int frame_rate_threshold_us,
                      const base::Closure& callback)
      : callback_(callback) {
    time_source_.reset(new viz::DelayBasedTimeSource(
        base::CreateSingleThreadTaskRunnerWithTraits(
            {content::BrowserThread::UI})
            .get()));
    time_source_->SetTimebaseAndInterval(
        base::TimeTicks(),
        base::TimeDelta::FromMicroseconds(frame_rate_threshold_us));
    time_source_->SetClient(this);
  }

  void SetActive(bool active) { time_source_->SetActive(active); }

  bool IsActive() const { return time_source_->Active(); }

  void SetFrameRateThresholdUs(int frame_rate_threshold_us) {
    time_source_->SetTimebaseAndInterval(
        base::TimeTicks::Now(),
        base::TimeDelta::FromMicroseconds(frame_rate_threshold_us));
  }

 private:
  void OnTimerTick() override { callback_.Run(); }

  const base::Closure callback_;
  std::unique_ptr<viz::DelayBasedTimeSource> time_source_;

  DISALLOW_COPY_AND_ASSIGN(AtomBeginFrameTimer);
};

#if !defined(OS_MACOSX)
class AtomDelegatedFrameHostClient : public content::DelegatedFrameHostClient {
 public:
  explicit AtomDelegatedFrameHostClient(OffScreenRenderWidgetHostView* view)
      : view_(view) {}

  ui::Layer* DelegatedFrameHostGetLayer() const override {
    return view_->GetRootLayer();
  }

  bool DelegatedFrameHostIsVisible() const override {
    return view_->IsShowing();
  }

  SkColor DelegatedFrameHostGetGutterColor() const override {
    if (view_->render_widget_host()->delegate() &&
        view_->render_widget_host()->delegate()->IsFullscreenForCurrentTab()) {
      return SK_ColorWHITE;
    }
    return *view_->GetBackgroundColor();
  }

  void OnFrameTokenChanged(uint32_t frame_token) override {
    view_->render_widget_host()->DidProcessFrame(frame_token);
  }

  float GetDeviceScaleFactor() const override {
    return view_->GetDeviceScaleFactor();
  }

  std::vector<viz::SurfaceId> CollectSurfaceIdsForEviction() override {
    return view_->render_widget_host()->CollectSurfaceIdsForEviction();
  }

  bool ShouldShowStaleContentOnEviction() override { return false; }

  void OnBeginFrame(base::TimeTicks frame_time) override {}
  void InvalidateLocalSurfaceIdOnEviction() override {}

 private:
  OffScreenRenderWidgetHostView* const view_;

  DISALLOW_COPY_AND_ASSIGN(AtomDelegatedFrameHostClient);
};
#endif  // !defined(OS_MACOSX)

OffScreenRenderWidgetHostView::OffScreenRenderWidgetHostView(
    bool transparent,
    bool painting,
    int frame_rate,
    const OnPaintCallback& callback,
    content::RenderWidgetHost* host,
    OffScreenRenderWidgetHostView* parent_host_view,
    gfx::Size initial_size)
    : content::RenderWidgetHostViewBase(host),
      render_widget_host_(content::RenderWidgetHostImpl::From(host)),
      parent_host_view_(parent_host_view),
      transparent_(transparent),
      callback_(callback),
      frame_rate_(frame_rate),
      size_(initial_size),
      painting_(painting),
      is_showing_(!render_widget_host_->is_hidden()),
      cursor_manager_(new content::CursorManager(this)),
      mouse_wheel_phase_handler_(this),
      weak_ptr_factory_(this) {
  DCHECK(render_widget_host_);
  bool is_guest_view_hack = parent_host_view_ != nullptr;

  current_device_scale_factor_ = kDefaultScaleFactor;

#if !defined(OS_MACOSX)
  local_surface_id_allocator_.GenerateId();
  local_surface_id_allocation_ =
      local_surface_id_allocator_.GetCurrentLocalSurfaceIdAllocation();
  delegated_frame_host_client_.reset(new AtomDelegatedFrameHostClient(this));
  delegated_frame_host_ = std::make_unique<content::DelegatedFrameHost>(
      AllocateFrameSinkId(is_guest_view_hack),
      delegated_frame_host_client_.get(),
      true /* should_register_frame_sink_id */);

  root_layer_.reset(new ui::Layer(ui::LAYER_SOLID_COLOR));
#endif

#if defined(OS_MACOSX)
  last_frame_root_background_color_ = SK_ColorTRANSPARENT;
  CreatePlatformWidget(is_guest_view_hack);
#endif

  bool opaque = SkColorGetA(background_color_) == SK_AlphaOPAQUE;
  GetRootLayer()->SetFillsBoundsOpaquely(opaque);
  GetRootLayer()->SetColor(background_color_);

#if !defined(OS_MACOSX)
  // On macOS the ui::Compositor is created/owned by the platform view.
  content::ImageTransportFactory* factory =
      content::ImageTransportFactory::GetInstance();
  ui::ContextFactoryPrivate* context_factory_private =
      factory->GetContextFactoryPrivate();
  compositor_.reset(new ui::Compositor(
      context_factory_private->AllocateFrameSinkId(),
      content::GetContextFactory(), context_factory_private,
      base::ThreadTaskRunnerHandle::Get(), false /* enable_pixel_canvas */));
  compositor_->SetAcceleratedWidget(gfx::kNullAcceleratedWidget);
  compositor_->SetRootLayer(root_layer_.get());
#endif
  GetCompositor()->SetDelegate(this);

  ResizeRootLayer(false);
  render_widget_host_->SetView(this);
  InstallTransparency();
}

OffScreenRenderWidgetHostView::~OffScreenRenderWidgetHostView() {
#if defined(OS_MACOSX)
  if (is_showing_)
    browser_compositor_->SetRenderWidgetHostIsHidden(true);
#else
  // Marking the DelegatedFrameHost as removed from the window hierarchy is
  // necessary to remove all connections to its old ui::Compositor.
  if (is_showing_)
    delegated_frame_host_->WasHidden();
  delegated_frame_host_->DetachFromCompositor();
#endif

  if (copy_frame_generator_.get())
    copy_frame_generator_.reset(NULL);

#if defined(OS_MACOSX)
  DestroyPlatformWidget();
#else
  delegated_frame_host_.reset(NULL);
  compositor_.reset(NULL);
  root_layer_.reset(NULL);
#endif
}

content::BrowserAccessibilityManager*
OffScreenRenderWidgetHostView::CreateBrowserAccessibilityManager(
    content::BrowserAccessibilityDelegate*,
    bool) {
  return nullptr;
}

void OffScreenRenderWidgetHostView::OnBeginFrameTimerTick() {
  const base::TimeTicks frame_time = base::TimeTicks::Now();
  const base::TimeDelta vsync_period =
      base::TimeDelta::FromMicroseconds(frame_rate_threshold_us_);
  SendBeginFrame(frame_time, vsync_period);
}

void OffScreenRenderWidgetHostView::SendBeginFrame(
    base::TimeTicks frame_time,
    base::TimeDelta vsync_period) {
  base::TimeTicks display_time = frame_time + vsync_period;

  base::TimeDelta estimated_browser_composite_time =
      base::TimeDelta::FromMicroseconds(
          (1.0f * base::Time::kMicrosecondsPerSecond) / (3.0f * 60));

  base::TimeTicks deadline = display_time - estimated_browser_composite_time;

  const viz::BeginFrameArgs& begin_frame_args = viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, begin_frame_source_.source_id(),
      begin_frame_number_, frame_time, deadline, vsync_period,
      viz::BeginFrameArgs::NORMAL);
  DCHECK(begin_frame_args.IsValid());
  begin_frame_number_++;

  if (renderer_compositor_frame_sink_)
    renderer_compositor_frame_sink_->OnBeginFrame(begin_frame_args, {});
}

void OffScreenRenderWidgetHostView::InitAsChild(gfx::NativeView) {
  DCHECK(parent_host_view_);

  if (parent_host_view_->child_host_view_) {
    parent_host_view_->child_host_view_->CancelWidget();
  }

  parent_host_view_->set_child_host_view(this);
  parent_host_view_->Hide();

  ResizeRootLayer(false);
  Show();
}

void OffScreenRenderWidgetHostView::SetSize(const gfx::Size& size) {
  size_ = size;
  SynchronizeVisualProperties();
}

void OffScreenRenderWidgetHostView::SetBounds(const gfx::Rect& new_bounds) {
  SetSize(new_bounds.size());
}

gfx::NativeView OffScreenRenderWidgetHostView::GetNativeView() const {
  return gfx::NativeView();
}

gfx::NativeViewAccessible
OffScreenRenderWidgetHostView::GetNativeViewAccessible() {
  return gfx::NativeViewAccessible();
}

ui::TextInputClient* OffScreenRenderWidgetHostView::GetTextInputClient() {
  return nullptr;
}

void OffScreenRenderWidgetHostView::Focus() {}

bool OffScreenRenderWidgetHostView::HasFocus() const {
  return false;
}

bool OffScreenRenderWidgetHostView::IsSurfaceAvailableForCopy() const {
  return GetDelegatedFrameHost()->CanCopyFromCompositingSurface();
}

void OffScreenRenderWidgetHostView::Show() {
  if (is_showing_)
    return;

  is_showing_ = true;

#if defined(OS_MACOSX)
  browser_compositor_->SetRenderWidgetHostIsHidden(false);
#else
  delegated_frame_host_->AttachToCompositor(compositor_.get());
  delegated_frame_host_->WasShown(
      GetLocalSurfaceIdAllocation().local_surface_id(),
      GetRootLayer()->bounds().size(), false);
#endif

  if (render_widget_host_)
    render_widget_host_->WasShown(false);
}

void OffScreenRenderWidgetHostView::Hide() {
  if (!is_showing_)
    return;

  if (render_widget_host_)
    render_widget_host_->WasHidden();

#if defined(OS_MACOSX)
  browser_compositor_->SetRenderWidgetHostIsHidden(true);
#else
  GetDelegatedFrameHost()->WasHidden();
  GetDelegatedFrameHost()->DetachFromCompositor();
#endif

  is_showing_ = false;
}

bool OffScreenRenderWidgetHostView::IsShowing() {
  return is_showing_;
}

void OffScreenRenderWidgetHostView::EnsureSurfaceSynchronizedForWebTest() {
  ++latest_capture_sequence_number_;
  SynchronizeVisualProperties();
}

gfx::Rect OffScreenRenderWidgetHostView::GetViewBounds() const {
  if (IsPopupWidget())
    return popup_position_;

  return gfx::Rect(size_);
}

void OffScreenRenderWidgetHostView::SetBackgroundColor(SkColor color) {
  // The renderer will feed its color back to us with the first CompositorFrame.
  // We short-cut here to show a sensible color before that happens.
  UpdateBackgroundColorFromRenderer(color);

  if (render_widget_host_ && render_widget_host_->owner_delegate()) {
    render_widget_host_->owner_delegate()->SetBackgroundOpaque(
        SkColorGetA(color) == SK_AlphaOPAQUE);
  }
}

base::Optional<SkColor> OffScreenRenderWidgetHostView::GetBackgroundColor()
    const {
  return background_color_;
}

void OffScreenRenderWidgetHostView::UpdateBackgroundColor() {
  NOTREACHED();
}

gfx::Size OffScreenRenderWidgetHostView::GetVisibleViewportSize() const {
  return size_;
}

void OffScreenRenderWidgetHostView::SetInsets(const gfx::Insets& insets) {}

bool OffScreenRenderWidgetHostView::LockMouse() {
  return false;
}

void OffScreenRenderWidgetHostView::UnlockMouse() {}

void OffScreenRenderWidgetHostView::TakeFallbackContentFrom(
    content::RenderWidgetHostView* view) {
  DCHECK(!static_cast<content::RenderWidgetHostViewBase*>(view)
              ->IsRenderWidgetHostViewChildFrame());
  DCHECK(!static_cast<content::RenderWidgetHostViewBase*>(view)
              ->IsRenderWidgetHostViewGuest());
  OffScreenRenderWidgetHostView* view_osr =
      static_cast<OffScreenRenderWidgetHostView*>(view);
  SetBackgroundColor(view_osr->background_color_);
  if (GetDelegatedFrameHost() && view_osr->GetDelegatedFrameHost()) {
    GetDelegatedFrameHost()->TakeFallbackContentFrom(
        view_osr->GetDelegatedFrameHost());
  }
  host()->GetContentRenderingTimeoutFrom(view_osr->host());
}

void OffScreenRenderWidgetHostView::DidCreateNewRendererCompositorFrameSink(
    viz::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink) {
  renderer_compositor_frame_sink_ = renderer_compositor_frame_sink;

#if defined(OS_MACOSX)
  browser_compositor_->DidCreateNewRendererCompositorFrameSink(
      renderer_compositor_frame_sink_);
#else
  if (GetDelegatedFrameHost()) {
    GetDelegatedFrameHost()->DidCreateNewRendererCompositorFrameSink(
        renderer_compositor_frame_sink_);
  }
#endif
}

void OffScreenRenderWidgetHostView::SubmitCompositorFrame(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame,
    base::Optional<viz::HitTestRegionList> hit_test_region_list) {
#if defined(OS_MACOSX)
  last_frame_root_background_color_ = frame.metadata.root_background_color;
#endif

  if (frame.metadata.root_scroll_offset != last_scroll_offset_) {
    last_scroll_offset_ = frame.metadata.root_scroll_offset;
  }

  if (!frame.render_pass_list.empty()) {
    if (software_output_device_) {
      if (!begin_frame_timer_.get() || IsPopupWidget()) {
        software_output_device_->SetActive(painting_, false);
      }

      // The compositor will draw directly to the SoftwareOutputDevice which
      // then calls OnPaint.
      // We would normally call BrowserCompositorMac::SubmitCompositorFrame on
      // macOS, however it contains compositor resize logic that we don't want.
      // Consequently we instead call the SubmitCompositorFrame method directly.
      GetDelegatedFrameHost()->SubmitCompositorFrame(
          local_surface_id, std::move(frame), std::move(hit_test_region_list));
    } else {
      if (!copy_frame_generator_.get()) {
        copy_frame_generator_.reset(
            new AtomCopyFrameGenerator(this, frame_rate_threshold_us_));
      }

      // Determine the damage rectangle for the current frame. This is the same
      // calculation that SwapDelegatedFrame uses.
      viz::RenderPass* root_pass = frame.render_pass_list.back().get();
      gfx::Size frame_size = root_pass->output_rect.size();
      gfx::Rect damage_rect =
          gfx::ToEnclosingRect(gfx::RectF(root_pass->damage_rect));
      damage_rect.Intersect(gfx::Rect(frame_size));

      // We would normally call BrowserCompositorMac::SubmitCompositorFrame on
      // macOS, however it contains compositor resize logic that we don't want.
      // Consequently we instead call the SubmitCompositorFrame method directly.
      GetDelegatedFrameHost()->SubmitCompositorFrame(
          local_surface_id, std::move(frame), std::move(hit_test_region_list));

      // Request a copy of the last compositor frame which will eventually call
      // OnPaint asynchronously.
      copy_frame_generator_->GenerateCopyFrame(damage_rect);
    }
  }
}

void OffScreenRenderWidgetHostView::ClearCompositorFrame() {
  NOTREACHED();
}

void OffScreenRenderWidgetHostView::ResetFallbackToFirstNavigationSurface() {
  GetDelegatedFrameHost()->ResetFallbackToFirstNavigationSurface();
}

void OffScreenRenderWidgetHostView::InitAsPopup(
    content::RenderWidgetHostView* parent_host_view,
    const gfx::Rect& pos) {
  DCHECK_EQ(parent_host_view_, parent_host_view);

  if (parent_host_view_->popup_host_view_) {
    parent_host_view_->popup_host_view_->CancelWidget();
  }

  parent_host_view_->set_popup_host_view(this);
  parent_host_view_->popup_bitmap_.reset(new SkBitmap);
  parent_callback_ =
      base::Bind(&OffScreenRenderWidgetHostView::OnPopupPaint,
                 parent_host_view_->weak_ptr_factory_.GetWeakPtr());

  popup_position_ = pos;

  ResizeRootLayer(false);
  Show();
}

void OffScreenRenderWidgetHostView::InitAsFullscreen(
    content::RenderWidgetHostView*) {}

void OffScreenRenderWidgetHostView::UpdateCursor(const content::WebCursor&) {}

content::CursorManager* OffScreenRenderWidgetHostView::GetCursorManager() {
  return cursor_manager_.get();
}

void OffScreenRenderWidgetHostView::SetIsLoading(bool loading) {}

void OffScreenRenderWidgetHostView::TextInputStateChanged(
    const content::TextInputState& params) {}

void OffScreenRenderWidgetHostView::ImeCancelComposition() {}

void OffScreenRenderWidgetHostView::RenderProcessGone(base::TerminationStatus,
                                                      int) {
  Destroy();
}

void OffScreenRenderWidgetHostView::Destroy() {
  if (!is_destroyed_) {
    is_destroyed_ = true;

    if (parent_host_view_ != NULL) {
      CancelWidget();
    } else {
      if (popup_host_view_)
        popup_host_view_->CancelWidget();
      popup_bitmap_.reset();
      if (child_host_view_)
        child_host_view_->CancelWidget();
      if (!guest_host_views_.empty()) {
        // Guest RWHVs will be destroyed when the associated RWHVGuest is
        // destroyed. This parent RWHV may be destroyed first, so disassociate
        // the guest RWHVs here without destroying them.
        for (auto* guest_host_view : guest_host_views_)
          guest_host_view->parent_host_view_ = nullptr;
        guest_host_views_.clear();
      }
      for (auto* proxy_view : proxy_views_)
        proxy_view->RemoveObserver();
      Hide();
    }
  }

  delete this;
}

void OffScreenRenderWidgetHostView::SetTooltipText(const base::string16&) {}

uint32_t OffScreenRenderWidgetHostView::GetCaptureSequenceNumber() const {
  return latest_capture_sequence_number_;
}

void OffScreenRenderWidgetHostView::CopyFromSurface(
    const gfx::Rect& src_rect,
    const gfx::Size& output_size,
    base::OnceCallback<void(const SkBitmap&)> callback) {
  GetDelegatedFrameHost()->CopyFromCompositingSurface(src_rect, output_size,
                                                      std::move(callback));
}

void OffScreenRenderWidgetHostView::GetScreenInfo(
    content::ScreenInfo* screen_info) const {
  screen_info->depth = 24;
  screen_info->depth_per_component = 8;
  screen_info->orientation_angle = 0;
  screen_info->device_scale_factor = current_device_scale_factor_;
  screen_info->orientation_type =
      content::SCREEN_ORIENTATION_VALUES_LANDSCAPE_PRIMARY;
  screen_info->rect = gfx::Rect(size_);
  screen_info->available_rect = gfx::Rect(size_);
}

void OffScreenRenderWidgetHostView::InitAsGuest(
    content::RenderWidgetHostView* parent_host_view,
    content::RenderWidgetHostViewGuest* guest_view) {
  parent_host_view_->AddGuestHostView(this);
}

void OffScreenRenderWidgetHostView::TransformPointToRootSurface(
    gfx::PointF* point) {}

gfx::Rect OffScreenRenderWidgetHostView::GetBoundsInRootWindow() {
  return gfx::Rect(size_);
}

viz::SurfaceId OffScreenRenderWidgetHostView::GetCurrentSurfaceId() const {
  return GetDelegatedFrameHost()
             ? GetDelegatedFrameHost()->GetCurrentSurfaceId()
             : viz::SurfaceId();
}

void OffScreenRenderWidgetHostView::ImeCompositionRangeChanged(
    const gfx::Range&,
    const std::vector<gfx::Rect>&) {}

gfx::Size OffScreenRenderWidgetHostView::GetCompositorViewportPixelSize()
    const {
  return gfx::ScaleToCeiledSize(GetRequestedRendererSize(),
                                current_device_scale_factor_);
}

content::RenderWidgetHostViewBase*
OffScreenRenderWidgetHostView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host,
    content::RenderWidgetHost* embedder_render_widget_host,
    content::WebContentsView* web_contents_view) {
  if (render_widget_host->GetView()) {
    return static_cast<content::RenderWidgetHostViewBase*>(
        render_widget_host->GetView());
  }

  OffScreenRenderWidgetHostView* embedder_host_view = nullptr;
  if (embedder_render_widget_host) {
    embedder_host_view = static_cast<OffScreenRenderWidgetHostView*>(
        embedder_render_widget_host->GetView());
  }

  return new OffScreenRenderWidgetHostView(
      transparent_, true, embedder_host_view->GetFrameRate(), callback_,
      render_widget_host, embedder_host_view, size());
}

const viz::FrameSinkId& OffScreenRenderWidgetHostView::GetFrameSinkId() const {
  return GetDelegatedFrameHost()->frame_sink_id();
}

void OffScreenRenderWidgetHostView::DidNavigate() {
  ResizeRootLayer(true);
#if defined(OS_MACOSX)
  browser_compositor_->DidNavigate();
#else
  if (delegated_frame_host_)
    delegated_frame_host_->DidNavigate();
#endif
}

bool OffScreenRenderWidgetHostView::TransformPointToLocalCoordSpaceLegacy(
    const gfx::PointF& point,
    const viz::SurfaceId& original_surface,
    gfx::PointF* transformed_point) {
  // Transformations use physical pixels rather than DIP, so conversion
  // is necessary.
  gfx::PointF point_in_pixels =
      gfx::ConvertPointToPixel(current_device_scale_factor_, point);
  if (!GetDelegatedFrameHost()->TransformPointToLocalCoordSpaceLegacy(
          point_in_pixels, original_surface, transformed_point)) {
    return false;
  }

  *transformed_point =
      gfx::ConvertPointToDIP(current_device_scale_factor_, *transformed_point);
  return true;
}

bool OffScreenRenderWidgetHostView::TransformPointToCoordSpaceForView(
    const gfx::PointF& point,
    RenderWidgetHostViewBase* target_view,
    gfx::PointF* transformed_point,
    viz::EventSource source) {
  if (target_view == this) {
    *transformed_point = point;
    return true;
  }

  return false;
}

void OffScreenRenderWidgetHostView::CancelWidget() {
  if (render_widget_host_)
    render_widget_host_->LostCapture();
  Hide();

  if (parent_host_view_) {
    if (parent_host_view_->popup_host_view_ == this) {
      parent_host_view_->set_popup_host_view(NULL);
      parent_host_view_->popup_bitmap_.reset();
    } else if (parent_host_view_->child_host_view_ == this) {
      parent_host_view_->set_child_host_view(NULL);
      parent_host_view_->Show();
    } else {
      parent_host_view_->RemoveGuestHostView(this);
    }
    parent_host_view_ = NULL;
  }

  if (render_widget_host_ && !is_destroyed_) {
    is_destroyed_ = true;
    // Results in a call to Destroy().
    render_widget_host_->ShutdownAndDestroyWidget(true);
  }
}

void OffScreenRenderWidgetHostView::AddGuestHostView(
    OffScreenRenderWidgetHostView* guest_host) {
  guest_host_views_.insert(guest_host);
}

void OffScreenRenderWidgetHostView::RemoveGuestHostView(
    OffScreenRenderWidgetHostView* guest_host) {
  guest_host_views_.erase(guest_host);
}

void OffScreenRenderWidgetHostView::AddViewProxy(OffscreenViewProxy* proxy) {
  proxy->SetObserver(this);
  proxy_views_.insert(proxy);
}

void OffScreenRenderWidgetHostView::RemoveViewProxy(OffscreenViewProxy* proxy) {
  proxy->RemoveObserver();
  proxy_views_.erase(proxy);
}

void OffScreenRenderWidgetHostView::ProxyViewDestroyed(
    OffscreenViewProxy* proxy) {
  proxy_views_.erase(proxy);
  Invalidate();
}

std::unique_ptr<viz::SoftwareOutputDevice>
OffScreenRenderWidgetHostView::CreateSoftwareOutputDevice(
    ui::Compositor* compositor) {
  DCHECK_EQ(GetCompositor(), compositor);
  DCHECK(!copy_frame_generator_);
  DCHECK(!software_output_device_);

  ResizeRootLayer(false);

  software_output_device_ = new OffScreenOutputDevice(
      transparent_, base::Bind(&OffScreenRenderWidgetHostView::OnPaint,
                               weak_ptr_factory_.GetWeakPtr()));
  return base::WrapUnique(software_output_device_);
}

bool OffScreenRenderWidgetHostView::InstallTransparency() {
  if (transparent_) {
    SetBackgroundColor(SkColor());
#if defined(OS_MACOSX)
    browser_compositor_->SetBackgroundColor(SK_ColorTRANSPARENT);
#else
    compositor_->SetBackgroundColor(SK_ColorTRANSPARENT);
#endif
    return true;
  }
  return false;
}

void OffScreenRenderWidgetHostView::SetNeedsBeginFrames(
    bool needs_begin_frames) {
  SetupFrameRate(true);

  begin_frame_timer_->SetActive(needs_begin_frames);

  if (software_output_device_) {
    software_output_device_->SetActive(painting_, false);
  }
}

void OffScreenRenderWidgetHostView::SetWantsAnimateOnlyBeginFrames() {
  if (GetDelegatedFrameHost()) {
    GetDelegatedFrameHost()->SetWantsAnimateOnlyBeginFrames();
  }
}

void OffScreenRenderWidgetHostView::OnPaint(const gfx::Rect& damage_rect,
                                            const SkBitmap& bitmap) {
  HoldResize();

  if (parent_callback_) {
    parent_callback_.Run(damage_rect, bitmap);
  } else {
    gfx::Rect damage(damage_rect);

    gfx::Size size_in_pixels = gfx::ConvertSizeToPixel(
        current_device_scale_factor_, GetViewBounds().size());

    SkBitmap backing;
    backing.allocN32Pixels(size_in_pixels.width(), size_in_pixels.height(),
                           false);
    SkCanvas canvas(backing);

    canvas.writePixels(bitmap, 0, 0);

    if (popup_host_view_ && popup_bitmap_.get()) {
      gfx::Rect rect = popup_host_view_->popup_position_;
      gfx::Point origin_in_pixels =
          gfx::ConvertPointToPixel(current_device_scale_factor_, rect.origin());
      damage.Union(rect);
      canvas.writePixels(*popup_bitmap_.get(), origin_in_pixels.x(),
                         origin_in_pixels.y());
    }

    for (auto* proxy_view : proxy_views_) {
      gfx::Rect rect = proxy_view->GetBounds();
      gfx::Point origin_in_pixels =
          gfx::ConvertPointToPixel(current_device_scale_factor_, rect.origin());
      damage.Union(rect);
      canvas.writePixels(*proxy_view->GetBitmap(), origin_in_pixels.x(),
                         origin_in_pixels.y());
    }

    damage.Intersect(GetViewBounds());
    paint_callback_running_ = true;
    callback_.Run(damage, backing);
    paint_callback_running_ = false;
  }

  ReleaseResize();
}

void OffScreenRenderWidgetHostView::OnPopupPaint(const gfx::Rect& damage_rect,
                                                 const SkBitmap& bitmap) {
  if (popup_host_view_ && popup_bitmap_.get())
    popup_bitmap_.reset(new SkBitmap(bitmap));
  InvalidateBounds(popup_host_view_->popup_position_);
}

void OffScreenRenderWidgetHostView::OnProxyViewPaint(
    const gfx::Rect& damage_rect) {
  InvalidateBounds(damage_rect);
}

void OffScreenRenderWidgetHostView::HoldResize() {
  if (!hold_resize_)
    hold_resize_ = true;
}

void OffScreenRenderWidgetHostView::ReleaseResize() {
  if (!hold_resize_)
    return;

  hold_resize_ = false;
  if (pending_resize_) {
    pending_resize_ = false;
    base::PostTaskWithTraits(
        FROM_HERE, {content::BrowserThread::UI},
        base::BindOnce(
            &OffScreenRenderWidgetHostView::SynchronizeVisualProperties,
            weak_ptr_factory_.GetWeakPtr()));
  }
}

void OffScreenRenderWidgetHostView::SynchronizeVisualProperties() {
  if (hold_resize_) {
    if (!pending_resize_)
      pending_resize_ = true;
    return;
  }

  ResizeRootLayer(false);
}

void OffScreenRenderWidgetHostView::SendMouseEvent(
    const blink::WebMouseEvent& event) {
  for (auto* proxy_view : proxy_views_) {
    gfx::Rect bounds = proxy_view->GetBounds();
    if (bounds.Contains(event.PositionInWidget().x,
                        event.PositionInWidget().y)) {
      blink::WebMouseEvent proxy_event(event);
      proxy_event.SetPositionInWidget(
          proxy_event.PositionInWidget().x - bounds.x(),
          proxy_event.PositionInWidget().y - bounds.y());

      ui::MouseEvent ui_event = UiMouseEventFromWebMouseEvent(proxy_event);
      proxy_view->OnEvent(&ui_event);
      return;
    }
  }

  if (!IsPopupWidget()) {
    if (popup_host_view_ &&
        popup_host_view_->popup_position_.Contains(
            event.PositionInWidget().x, event.PositionInWidget().y)) {
      blink::WebMouseEvent popup_event(event);
      popup_event.SetPositionInWidget(
          popup_event.PositionInWidget().x -
              popup_host_view_->popup_position_.x(),
          popup_event.PositionInWidget().y -
              popup_host_view_->popup_position_.y());

      popup_host_view_->ProcessMouseEvent(popup_event, ui::LatencyInfo());
      return;
    }
  }

  if (!render_widget_host_)
    return;
  render_widget_host_->ForwardMouseEvent(event);
}

void OffScreenRenderWidgetHostView::SendMouseWheelEvent(
    const blink::WebMouseWheelEvent& event) {
  for (auto* proxy_view : proxy_views_) {
    gfx::Rect bounds = proxy_view->GetBounds();
    if (bounds.Contains(event.PositionInWidget().x,
                        event.PositionInWidget().y)) {
      blink::WebMouseWheelEvent proxy_event(event);
      proxy_event.SetPositionInWidget(
          proxy_event.PositionInWidget().x - bounds.x(),
          proxy_event.PositionInWidget().y - bounds.y());

      ui::MouseWheelEvent ui_event =
          UiMouseWheelEventFromWebMouseEvent(proxy_event);
      proxy_view->OnEvent(&ui_event);
      return;
    }
  }

  blink::WebMouseWheelEvent mouse_wheel_event(event);

  bool should_route_event =
      render_widget_host_->delegate() &&
      render_widget_host_->delegate()->GetInputEventRouter();
  mouse_wheel_phase_handler_.SendWheelEndForTouchpadScrollingIfNeeded(
      should_route_event);
  mouse_wheel_phase_handler_.AddPhaseIfNeededAndScheduleEndEvent(
      mouse_wheel_event, false);

  if (!IsPopupWidget()) {
    if (popup_host_view_) {
      if (popup_host_view_->popup_position_.Contains(
              mouse_wheel_event.PositionInWidget().x,
              mouse_wheel_event.PositionInWidget().y)) {
        blink::WebMouseWheelEvent popup_mouse_wheel_event(mouse_wheel_event);
        popup_mouse_wheel_event.SetPositionInWidget(
            mouse_wheel_event.PositionInWidget().x -
                popup_host_view_->popup_position_.x(),
            mouse_wheel_event.PositionInWidget().y -
                popup_host_view_->popup_position_.y());
        popup_mouse_wheel_event.SetPositionInScreen(
            popup_mouse_wheel_event.PositionInWidget().x,
            popup_mouse_wheel_event.PositionInWidget().y);

        popup_host_view_->SendMouseWheelEvent(popup_mouse_wheel_event);
        return;
      } else {
        // Scrolling outside of the popup widget so destroy it.
        // Execute asynchronously to avoid deleting the widget from inside some
        // other callback.
        base::PostTaskWithTraits(
            FROM_HERE, {content::BrowserThread::UI},
            base::BindOnce(&OffScreenRenderWidgetHostView::CancelWidget,
                           popup_host_view_->weak_ptr_factory_.GetWeakPtr()));
      }
    } else if (!guest_host_views_.empty()) {
      for (auto* guest_host_view : guest_host_views_) {
        if (!guest_host_view->render_widget_host_ ||
            !guest_host_view->render_widget_host_->GetView()) {
          continue;
        }
        const gfx::Rect& guest_bounds =
            guest_host_view->render_widget_host_->GetView()->GetViewBounds();
        if (guest_bounds.Contains(mouse_wheel_event.PositionInWidget().x,
                                  mouse_wheel_event.PositionInWidget().y)) {
          blink::WebMouseWheelEvent guest_mouse_wheel_event(mouse_wheel_event);
          guest_mouse_wheel_event.SetPositionInWidget(
              mouse_wheel_event.PositionInWidget().x - guest_bounds.x(),
              mouse_wheel_event.PositionInWidget().y - guest_bounds.y());
          guest_mouse_wheel_event.SetPositionInScreen(
              guest_mouse_wheel_event.PositionInWidget().x,
              guest_mouse_wheel_event.PositionInWidget().y);

          guest_host_view->SendMouseWheelEvent(guest_mouse_wheel_event);
          return;
        }
      }
    }
  }
  if (!render_widget_host_)
    return;
  render_widget_host_->ForwardWheelEvent(event);
}

void OffScreenRenderWidgetHostView::SetPainting(bool painting) {
  painting_ = painting;

  if (software_output_device_) {
    software_output_device_->SetActive(painting_, !paint_callback_running_);
  }
}

bool OffScreenRenderWidgetHostView::IsPainting() const {
  return painting_;
}

void OffScreenRenderWidgetHostView::SetFrameRate(int frame_rate) {
  if (parent_host_view_) {
    if (parent_host_view_->GetFrameRate() == GetFrameRate())
      return;

    frame_rate_ = parent_host_view_->GetFrameRate();
  } else {
    if (frame_rate <= 0)
      frame_rate = 1;
    if (frame_rate > 240)
      frame_rate = 240;

    frame_rate_ = frame_rate;
  }

  SetupFrameRate(true);

  for (auto* guest_host_view : guest_host_views_)
    guest_host_view->SetFrameRate(frame_rate);
}

int OffScreenRenderWidgetHostView::GetFrameRate() const {
  return frame_rate_;
}

#if !defined(OS_MACOSX)
ui::Compositor* OffScreenRenderWidgetHostView::GetCompositor() const {
  return compositor_.get();
}

ui::Layer* OffScreenRenderWidgetHostView::GetRootLayer() const {
  return root_layer_.get();
}

#if !defined(OS_MACOSX)
const viz::LocalSurfaceIdAllocation&
OffScreenRenderWidgetHostView::GetLocalSurfaceIdAllocation() const {
  return local_surface_id_allocation_;
}
#endif  // defined(OS_MACOSX)

content::DelegatedFrameHost*
OffScreenRenderWidgetHostView::GetDelegatedFrameHost() const {
  return delegated_frame_host_.get();
}
#endif

void OffScreenRenderWidgetHostView::SetupFrameRate(bool force) {
  if (!force && frame_rate_threshold_us_ != 0)
    return;

  frame_rate_threshold_us_ = 1000000 / frame_rate_;

  if (copy_frame_generator_.get()) {
    copy_frame_generator_->set_frame_rate_threshold_us(
        frame_rate_threshold_us_);
  }

  if (begin_frame_timer_.get()) {
    begin_frame_timer_->SetFrameRateThresholdUs(frame_rate_threshold_us_);
  } else {
    begin_frame_timer_.reset(new AtomBeginFrameTimer(
        frame_rate_threshold_us_,
        base::Bind(&OffScreenRenderWidgetHostView::OnBeginFrameTimerTick,
                   weak_ptr_factory_.GetWeakPtr())));
  }
}

void OffScreenRenderWidgetHostView::Invalidate() {
  InvalidateBounds(GetViewBounds());
}

void OffScreenRenderWidgetHostView::InvalidateBounds(const gfx::Rect& bounds) {
  if (software_output_device_) {
    software_output_device_->OnPaint(bounds);
  } else if (copy_frame_generator_) {
    copy_frame_generator_->GenerateCopyFrame(bounds);
  }
}

void OffScreenRenderWidgetHostView::ResizeRootLayer(bool force) {
  SetupFrameRate(false);

  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestView(GetNativeView());
  const float scaleFactor = display.device_scale_factor();
  const bool scaleFactorDidChange =
      (scaleFactor != current_device_scale_factor_);

  current_device_scale_factor_ = scaleFactor;

  gfx::Size size;
  if (!IsPopupWidget())
    size = GetViewBounds().size();
  else
    size = popup_position_.size();

  if (!force && !scaleFactorDidChange &&
      size == GetRootLayer()->bounds().size())
    return;

  GetRootLayer()->SetBounds(gfx::Rect(size));

#if defined(OS_MACOSX)
  bool resized = UpdateNSViewAndDisplay();
#else
  const gfx::Size& size_in_pixels =
      gfx::ConvertSizeToPixel(current_device_scale_factor_, size);

  local_surface_id_allocator_.GenerateId();
  local_surface_id_allocation_ =
      local_surface_id_allocator_.GetCurrentLocalSurfaceIdAllocation();

  GetCompositor()->SetScaleAndSize(current_device_scale_factor_, size_in_pixels,
                                   local_surface_id_allocation_);
  bool resized = true;
  GetDelegatedFrameHost()->EmbedSurface(
      local_surface_id_allocation_.local_surface_id(), size,
      cc::DeadlinePolicy::UseDefaultDeadline());
#endif

  // Note that |render_widget_host_| will retrieve resize parameters from the
  // DelegatedFrameHost, so it must have SynchronizeVisualProperties called
  // after.
  if (resized && render_widget_host_) {
    render_widget_host_->SynchronizeVisualProperties();
  }
}

viz::FrameSinkId OffScreenRenderWidgetHostView::AllocateFrameSinkId(
    bool is_guest_view_hack) {
  // GuestViews have two RenderWidgetHostViews and so we need to make sure
  // we don't have FrameSinkId collisions.
  // The FrameSinkId generated here must be unique with FrameSinkId allocated
  // in ContextFactoryPrivate.
  content::ImageTransportFactory* factory =
      content::ImageTransportFactory::GetInstance();
  return is_guest_view_hack
             ? factory->GetContextFactoryPrivate()->AllocateFrameSinkId()
             : viz::FrameSinkId(base::checked_cast<uint32_t>(
                                    render_widget_host_->GetProcess()->GetID()),
                                base::checked_cast<uint32_t>(
                                    render_widget_host_->GetRoutingID()));
}

void OffScreenRenderWidgetHostView::UpdateBackgroundColorFromRenderer(
    SkColor color) {
  if (color == background_color_)
    return;
  background_color_ = color;

  bool opaque = SkColorGetA(color) == SK_AlphaOPAQUE;
  GetRootLayer()->SetFillsBoundsOpaquely(opaque);
  GetRootLayer()->SetColor(color);
}

}  // namespace atom
