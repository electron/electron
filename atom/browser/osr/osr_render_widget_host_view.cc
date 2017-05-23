// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_render_widget_host_view.h"

#include <algorithm>
#include <vector>

#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "cc/output/copy_output_request.h"
#include "cc/scheduler/delay_based_time_source.h"
#include "components/display_compositor/gl_helper.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_frame_subscriber.h"
#include "content/browser/renderer_host/resize_lock.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/context_factory.h"
#include "media/base/video_frame.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_type.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event_constants.h"
#include "ui/events/latency_info.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/skbitmap_operations.h"

namespace atom {

namespace {

const float kDefaultScaleFactor = 1.0;
const int kFrameRetryLimit = 2;

ui::MouseEvent UiMouseEventFromWebMouseEvent(blink::WebMouseEvent event) {
  ui::EventType type = ui::EventType::ET_UNKNOWN;
  switch (event.type()) {
    case blink::WebInputEvent::MouseDown:
      type = ui::EventType::ET_MOUSE_PRESSED;
      break;
    case blink::WebInputEvent::MouseUp:
      type = ui::EventType::ET_MOUSE_RELEASED;
      break;
    case blink::WebInputEvent::MouseMove:
      type = ui::EventType::ET_MOUSE_MOVED;
      break;
    case blink::WebInputEvent::MouseEnter:
      type = ui::EventType::ET_MOUSE_ENTERED;
      break;
    case blink::WebInputEvent::MouseLeave:
      type = ui::EventType::ET_MOUSE_EXITED;
      break;
    case blink::WebInputEvent::MouseWheel:
      type = ui::EventType::ET_MOUSEWHEEL;
      break;
    default:
      type = ui::EventType::ET_UNKNOWN;
      break;
  }

  int button_flags = 0;
  switch (event.button) {
    case blink::WebMouseEvent::Button::X1:
      button_flags |= ui::EventFlags::EF_BACK_MOUSE_BUTTON;
      break;
    case blink::WebMouseEvent::Button::X2:
      button_flags |= ui::EventFlags::EF_FORWARD_MOUSE_BUTTON;
      break;
    case blink::WebMouseEvent::Button::Left:
      button_flags |= ui::EventFlags::EF_LEFT_MOUSE_BUTTON;
      break;
    case blink::WebMouseEvent::Button::Middle:
      button_flags |= ui::EventFlags::EF_MIDDLE_MOUSE_BUTTON;
      break;
    case blink::WebMouseEvent::Button::Right:
      button_flags |= ui::EventFlags::EF_RIGHT_MOUSE_BUTTON;
      break;
    default:
      button_flags = 0;
      break;
  }

  ui::MouseEvent ui_event(type,
    gfx::Point(std::floor(event.x), std::floor(event.y)),
    gfx::Point(std::floor(event.x), std::floor(event.y)),
    ui::EventTimeForNow(),
    button_flags, button_flags);
  ui_event.SetClickCount(event.clickCount);

  return ui_event;
}

ui::MouseWheelEvent UiMouseWheelEventFromWebMouseEvent(
    blink::WebMouseWheelEvent event) {
  return ui::MouseWheelEvent(UiMouseEventFromWebMouseEvent(event),
    std::floor(event.deltaX), std::floor(event.deltaY));
}

#if !defined(OS_MACOSX)

const int kResizeLockTimeoutMs = 67;

class AtomResizeLock : public content::ResizeLock {
 public:
  AtomResizeLock(OffScreenRenderWidgetHostView* host,
                const gfx::Size new_size,
                bool defer_compositor_lock)
      : ResizeLock(new_size, defer_compositor_lock),
        host_(host),
        cancelled_(false),
        weak_ptr_factory_(this) {
    DCHECK(host_);
    host_->HoldResize();

    content::BrowserThread::PostDelayedTask(content::BrowserThread::UI,
      FROM_HERE, base::Bind(&AtomResizeLock::CancelLock,
        weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(kResizeLockTimeoutMs));
  }

  ~AtomResizeLock() override {
    CancelLock();
  }

  bool GrabDeferredLock() override {
    return ResizeLock::GrabDeferredLock();
  }

  void UnlockCompositor() override {
    ResizeLock::UnlockCompositor();
    compositor_lock_ = NULL;
  }

 protected:
  void LockCompositor() override {
    ResizeLock::LockCompositor();
    compositor_lock_ = host_->GetCompositor()->GetCompositorLock();
  }

  void CancelLock() {
    if (cancelled_)
      return;
    cancelled_ = true;
    UnlockCompositor();
    host_->ReleaseResize();
  }

 private:
  OffScreenRenderWidgetHostView* host_;
  scoped_refptr<ui::CompositorLock> compositor_lock_;
  bool cancelled_;
  base::WeakPtrFactory<AtomResizeLock> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomResizeLock);
};

#endif  // !defined(OS_MACOSX)

}  // namespace

class AtomCopyFrameGenerator {
 public:
  AtomCopyFrameGenerator(OffScreenRenderWidgetHostView* view,
                         int frame_rate_threshold_us)
    : view_(view),
      frame_retry_count_(0),
      next_frame_time_(base::TimeTicks::Now()),
      frame_duration_(base::TimeDelta::FromMicroseconds(
        frame_rate_threshold_us)),
      weak_ptr_factory_(this) {
    last_time_ = base::Time::Now();
  }

  void GenerateCopyFrame(const gfx::Rect& damage_rect) {
    if (!view_->render_widget_host())
      return;

    std::unique_ptr<cc::CopyOutputRequest> request =
        cc::CopyOutputRequest::CreateBitmapRequest(base::Bind(
            &AtomCopyFrameGenerator::CopyFromCompositingSurfaceHasResult,
            weak_ptr_factory_.GetWeakPtr(),
            damage_rect));

    request->set_area(gfx::Rect(view_->GetPhysicalBackingSize()));
    view_->GetRootLayer()->RequestCopyOfOutput(std::move(request));
  }

  void set_frame_rate_threshold_us(int frame_rate_threshold_us) {
    frame_duration_ = base::TimeDelta::FromMicroseconds(
      frame_rate_threshold_us);
  }

 private:
  void CopyFromCompositingSurfaceHasResult(
      const gfx::Rect& damage_rect,
      std::unique_ptr<cc::CopyOutputResult> result) {
    if (result->IsEmpty() || result->size().IsEmpty() ||
        !view_->render_widget_host()) {
      OnCopyFrameCaptureFailure(damage_rect);
      return;
    }

    DCHECK(result->HasBitmap());
    std::unique_ptr<SkBitmap> source = result->TakeBitmap();
    DCHECK(source);
    if (source) {
      base::AutoLock autolock(lock_);
      std::shared_ptr<SkBitmap> bitmap(source.release());

      base::TimeTicks now = base::TimeTicks::Now();
      base::TimeDelta next_frame_in = next_frame_time_ - now;
      if (next_frame_in > frame_duration_ / 4) {
        next_frame_time_ += frame_duration_;
        content::BrowserThread::PostDelayedTask(content::BrowserThread::UI,
          FROM_HERE,
          base::Bind(&AtomCopyFrameGenerator::OnCopyFrameCaptureSuccess,
            weak_ptr_factory_.GetWeakPtr(),
            damage_rect,
            bitmap),
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
      content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
        base::Bind(&AtomCopyFrameGenerator::GenerateCopyFrame,
          weak_ptr_factory_.GetWeakPtr(),
          damage_rect));
    }
  }

  void OnCopyFrameCaptureSuccess(
      const gfx::Rect& damage_rect,
      std::shared_ptr<SkBitmap> bitmap) {
    base::AutoLock lock(onPaintLock_);
    view_->OnPaint(damage_rect, *bitmap);
  }

  base::Lock lock_;
  base::Lock onPaintLock_;
  OffScreenRenderWidgetHostView* view_;

  base::Time last_time_;

  int frame_retry_count_;
  base::TimeTicks next_frame_time_;
  base::TimeDelta frame_duration_;

  base::WeakPtrFactory<AtomCopyFrameGenerator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomCopyFrameGenerator);
};

class AtomBeginFrameTimer : public cc::DelayBasedTimeSourceClient {
 public:
  AtomBeginFrameTimer(int frame_rate_threshold_us,
                      const base::Closure& callback)
      : callback_(callback) {
    time_source_.reset(new cc::DelayBasedTimeSource(
        content::BrowserThread::GetTaskRunnerForThread(
            content::BrowserThread::UI).get()));
    time_source_->SetTimebaseAndInterval(
         base::TimeTicks(),
         base::TimeDelta::FromMicroseconds(frame_rate_threshold_us));
    time_source_->SetClient(this);
  }

  void SetActive(bool active) {
    time_source_->SetActive(active);
  }

  bool IsActive() const {
    return time_source_->Active();
  }

  void SetFrameRateThresholdUs(int frame_rate_threshold_us) {
    time_source_->SetTimebaseAndInterval(
        base::TimeTicks::Now(),
        base::TimeDelta::FromMicroseconds(frame_rate_threshold_us));
  }

 private:
  void OnTimerTick() override {
    callback_.Run();
  }

  const base::Closure callback_;
  std::unique_ptr<cc::DelayBasedTimeSource> time_source_;

  DISALLOW_COPY_AND_ASSIGN(AtomBeginFrameTimer);
};

OffScreenRenderWidgetHostView::OffScreenRenderWidgetHostView(
    bool transparent,
    const OnPaintCallback& callback,
    content::RenderWidgetHost* host,
    OffScreenRenderWidgetHostView* parent_host_view,
    NativeWindow* native_window)
    : render_widget_host_(content::RenderWidgetHostImpl::From(host)),
      parent_host_view_(parent_host_view),
      popup_host_view_(nullptr),
      child_host_view_(nullptr),
      native_window_(native_window),
      software_output_device_(nullptr),
      transparent_(transparent),
      callback_(callback),
      parent_callback_(nullptr),
      frame_rate_(60),
      frame_rate_threshold_us_(0),
      last_time_(base::Time::Now()),
      scale_factor_(kDefaultScaleFactor),
      size_(native_window->GetSize()),
      painting_(true),
      is_showing_(!render_widget_host_->is_hidden()),
      is_destroyed_(false),
      popup_position_(gfx::Rect()),
      hold_resize_(false),
      pending_resize_(false),
      weak_ptr_factory_(this) {
  DCHECK(render_widget_host_);
  bool is_guest_view_hack = parent_host_view_ != nullptr;
#if !defined(OS_MACOSX)
  delegated_frame_host_ = base::MakeUnique<content::DelegatedFrameHost>(
      AllocateFrameSinkId(is_guest_view_hack), this);

  root_layer_.reset(new ui::Layer(ui::LAYER_SOLID_COLOR));
#endif

#if defined(OS_MACOSX)
  CreatePlatformWidget(is_guest_view_hack);
#else
  // On macOS the ui::Compositor is created/owned by the platform view.
  content::ImageTransportFactory* factory =
      content::ImageTransportFactory::GetInstance();
  ui::ContextFactoryPrivate* context_factory_private =
      factory->GetContextFactoryPrivate();
  compositor_.reset(
      new ui::Compositor(context_factory_private->AllocateFrameSinkId(),
                         content::GetContextFactory(), context_factory_private,
                         base::ThreadTaskRunnerHandle::Get()));
  compositor_->SetAcceleratedWidget(native_window_->GetAcceleratedWidget());
  compositor_->SetRootLayer(root_layer_.get());
#endif
  GetCompositor()->SetDelegate(this);

  native_window_->AddObserver(this);

  ResizeRootLayer();
  render_widget_host_->SetView(this);
  InstallTransparency();
}

OffScreenRenderWidgetHostView::~OffScreenRenderWidgetHostView() {
  if (native_window_)
    native_window_->RemoveObserver(this);

#if defined(OS_MACOSX)
  if (is_showing_)
    browser_compositor_->SetRenderWidgetHostIsHidden(true);
#else
  // Marking the DelegatedFrameHost as removed from the window hierarchy is
  // necessary to remove all connections to its old ui::Compositor.
  if (is_showing_)
    delegated_frame_host_->WasHidden();
  delegated_frame_host_->ResetCompositor();
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

void OffScreenRenderWidgetHostView::OnWindowResize() {
  // In offscreen mode call RenderWidgetHostView's SetSize explicitly
  auto size = native_window_->GetSize();
  SetSize(size);
}

void OffScreenRenderWidgetHostView::OnWindowClosed() {
  native_window_->RemoveObserver(this);
  native_window_ = nullptr;
}

void OffScreenRenderWidgetHostView::OnBeginFrameTimerTick() {
  const base::TimeTicks frame_time = base::TimeTicks::Now();
  const base::TimeDelta vsync_period =
      base::TimeDelta::FromMicroseconds(frame_rate_threshold_us_);
  SendBeginFrame(frame_time, vsync_period);
}

void OffScreenRenderWidgetHostView::SendBeginFrame(
    base::TimeTicks frame_time, base::TimeDelta vsync_period) {
  base::TimeTicks display_time = frame_time + vsync_period;

  base::TimeDelta estimated_browser_composite_time =
      base::TimeDelta::FromMicroseconds(
          (1.0f * base::Time::kMicrosecondsPerSecond) / (3.0f * 60));

  base::TimeTicks deadline = display_time - estimated_browser_composite_time;

  const cc::BeginFrameArgs& begin_frame_args =
      cc::BeginFrameArgs::Create(BEGINFRAME_FROM_HERE,
                                 begin_frame_source_.source_id(),
                                 begin_frame_number_, frame_time, deadline,
                                 vsync_period, cc::BeginFrameArgs::NORMAL);
  DCHECK(begin_frame_args.IsValid());
  begin_frame_number_++;

  render_widget_host_->Send(new ViewMsg_BeginFrame(
      render_widget_host_->GetRoutingID(),
      begin_frame_args));
}

bool OffScreenRenderWidgetHostView::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(OffScreenRenderWidgetHostView, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_SetNeedsBeginFrames,
                        SetNeedsBeginFrames)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  if (!handled)
    return content::RenderWidgetHostViewBase::OnMessageReceived(message);
  return handled;
}

void OffScreenRenderWidgetHostView::InitAsChild(gfx::NativeView) {
  DCHECK(parent_host_view_);

  if (parent_host_view_->child_host_view_) {
    parent_host_view_->child_host_view_->CancelWidget();
  }

  parent_host_view_->set_child_host_view(this);
  parent_host_view_->Hide();

  ResizeRootLayer();
  Show();
}

content::RenderWidgetHost* OffScreenRenderWidgetHostView::GetRenderWidgetHost()
    const {
  return render_widget_host_;
}

void OffScreenRenderWidgetHostView::SetSize(const gfx::Size& size) {
  size_ = size;
  WasResized();
}

void OffScreenRenderWidgetHostView::SetBounds(const gfx::Rect& new_bounds) {
  SetSize(new_bounds.size());
}

gfx::Vector2dF OffScreenRenderWidgetHostView::GetLastScrollOffset() const {
  return last_scroll_offset_;
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

void OffScreenRenderWidgetHostView::Focus() {
}

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
  delegated_frame_host_->SetCompositor(compositor_.get());
  delegated_frame_host_->WasShown(ui::LatencyInfo());
#endif

  if (render_widget_host_)
    render_widget_host_->WasShown(ui::LatencyInfo());
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
  GetDelegatedFrameHost()->ResetCompositor();
#endif

  is_showing_ = false;
}

bool OffScreenRenderWidgetHostView::IsShowing() {
  return is_showing_;
}

gfx::Rect OffScreenRenderWidgetHostView::GetViewBounds() const {
  if (IsPopupWidget())
    return popup_position_;

  return gfx::Rect(size_);
}

void OffScreenRenderWidgetHostView::SetBackgroundColor(SkColor color) {
  if (transparent_)
    color = SkColorSetARGB(SK_AlphaTRANSPARENT, 0, 0, 0);

  content::RenderWidgetHostViewBase::SetBackgroundColor(color);

  const bool opaque = !transparent_ && GetBackgroundOpaque();
  if (render_widget_host_)
    render_widget_host_->SetBackgroundOpaque(opaque);
}

gfx::Size OffScreenRenderWidgetHostView::GetVisibleViewportSize() const {
  return size_;
}

void OffScreenRenderWidgetHostView::SetInsets(const gfx::Insets& insets) {
}

bool OffScreenRenderWidgetHostView::LockMouse() {
  return false;
}

void OffScreenRenderWidgetHostView::UnlockMouse() {
}

void OffScreenRenderWidgetHostView::OnSwapCompositorFrame(
  uint32_t output_surface_id,
  cc::CompositorFrame frame) {
  TRACE_EVENT0("electron",
    "OffScreenRenderWidgetHostView::OnSwapCompositorFrame");

  if (frame.metadata.root_scroll_offset != last_scroll_offset_) {
    last_scroll_offset_ = frame.metadata.root_scroll_offset;
  }

  if (!frame.render_pass_list.empty()) {
    if (software_output_device_) {
      if (!begin_frame_timer_.get() || IsPopupWidget()) {
        software_output_device_->SetActive(painting_);
      }

      // The compositor will draw directly to the SoftwareOutputDevice which
      // then calls OnPaint.
      // We would normally call BrowserCompositorMac::SwapCompositorFrame on
      // macOS, however it contains compositor resize logic that we don't want.
      // Consequently we instead call the SwapDelegatedFrame method directly.
      GetDelegatedFrameHost()->SwapDelegatedFrame(output_surface_id,
                                                  std::move(frame));
    } else {
      if (!copy_frame_generator_.get()) {
        copy_frame_generator_.reset(
            new AtomCopyFrameGenerator(this, frame_rate_threshold_us_));
      }

      // Determine the damage rectangle for the current frame. This is the same
      // calculation that SwapDelegatedFrame uses.
      cc::RenderPass* root_pass = frame.render_pass_list.back().get();
      gfx::Size frame_size = root_pass->output_rect.size();
      gfx::Rect damage_rect =
          gfx::ToEnclosingRect(gfx::RectF(root_pass->damage_rect));
      damage_rect.Intersect(gfx::Rect(frame_size));

      // We would normally call BrowserCompositorMac::SwapCompositorFrame on
      // macOS, however it contains compositor resize logic that we don't want.
      // Consequently we instead call the SwapDelegatedFrame method directly.
      GetDelegatedFrameHost()->SwapDelegatedFrame(output_surface_id,
                                                  std::move(frame));

      // Request a copy of the last compositor frame which will eventually call
      // OnPaint asynchronously.
      copy_frame_generator_->GenerateCopyFrame(damage_rect);
    }
  }
}

void OffScreenRenderWidgetHostView::ClearCompositorFrame() {
  GetDelegatedFrameHost()->ClearDelegatedFrame();
}

void OffScreenRenderWidgetHostView::InitAsPopup(
    content::RenderWidgetHostView* parent_host_view, const gfx::Rect& pos) {
  DCHECK_EQ(parent_host_view_, parent_host_view);

  if (parent_host_view_->popup_host_view_) {
    parent_host_view_->popup_host_view_->CancelWidget();
  }

  parent_host_view_->set_popup_host_view(this);
  parent_host_view_->popup_bitmap_.reset(new SkBitmap);
  parent_callback_ = base::Bind(&OffScreenRenderWidgetHostView::OnPopupPaint,
      parent_host_view_->weak_ptr_factory_.GetWeakPtr());

  popup_position_ = pos;

  ResizeRootLayer();
  Show();
}

void OffScreenRenderWidgetHostView::InitAsFullscreen(
    content::RenderWidgetHostView *) {
}

void OffScreenRenderWidgetHostView::UpdateCursor(const content::WebCursor &) {
}

void OffScreenRenderWidgetHostView::SetIsLoading(bool loading) {
}

void OffScreenRenderWidgetHostView::TextInputStateChanged(
  const content::TextInputState& params) {
}

void OffScreenRenderWidgetHostView::ImeCancelComposition() {
}

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
      for (auto guest_host_view : guest_host_views_)
        guest_host_view->CancelWidget();
      for (auto proxy_view : proxy_views_)
        proxy_view->RemoveObserver();
      Hide();
    }
  }

  delete this;
}

void OffScreenRenderWidgetHostView::SetTooltipText(const base::string16 &) {
}

void OffScreenRenderWidgetHostView::SelectionBoundsChanged(
  const ViewHostMsg_SelectionBounds_Params &) {
}

void OffScreenRenderWidgetHostView::CopyFromSurface(
    const gfx::Rect& src_subrect,
    const gfx::Size& dst_size,
    const content::ReadbackRequestCallback& callback,
    const SkColorType preferred_color_type) {
  GetDelegatedFrameHost()->CopyFromCompositingSurface(
    src_subrect, dst_size, callback, preferred_color_type);
}

void OffScreenRenderWidgetHostView::CopyFromSurfaceToVideoFrame(
    const gfx::Rect& src_subrect,
    scoped_refptr<media::VideoFrame> target,
    const base::Callback<void(const gfx::Rect&, bool)>& callback) {
  GetDelegatedFrameHost()->CopyFromCompositingSurfaceToVideoFrame(
    src_subrect, target, callback);
}

void OffScreenRenderWidgetHostView::BeginFrameSubscription(
  std::unique_ptr<content::RenderWidgetHostViewFrameSubscriber> subscriber) {
  GetDelegatedFrameHost()->BeginFrameSubscription(std::move(subscriber));
}

void OffScreenRenderWidgetHostView::EndFrameSubscription() {
  GetDelegatedFrameHost()->EndFrameSubscription();
}

void OffScreenRenderWidgetHostView::InitAsGuest(
    content::RenderWidgetHostView* parent_host_view,
    content::RenderWidgetHostViewGuest* guest_view) {
  parent_host_view_->AddGuestHostView(this);
  parent_host_view_->RegisterGuestViewFrameSwappedCallback(guest_view);
}

bool OffScreenRenderWidgetHostView::HasAcceleratedSurface(const gfx::Size &) {
  return false;
}

gfx::Rect OffScreenRenderWidgetHostView::GetBoundsInRootWindow() {
  return gfx::Rect(size_);
}

void OffScreenRenderWidgetHostView::ImeCompositionRangeChanged(
  const gfx::Range &, const std::vector<gfx::Rect>&) {
}

gfx::Size OffScreenRenderWidgetHostView::GetPhysicalBackingSize() const {
  return gfx::ConvertSizeToPixel(scale_factor_, GetRequestedRendererSize());
}

gfx::Size OffScreenRenderWidgetHostView::GetRequestedRendererSize() const {
  return GetDelegatedFrameHost()->GetRequestedRendererSize();
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
      transparent_,
      callback_,
      render_widget_host,
      embedder_host_view,
      native_window_);
}

#if !defined(OS_MACOSX)
ui::Layer* OffScreenRenderWidgetHostView::DelegatedFrameHostGetLayer() const {
  return const_cast<ui::Layer*>(root_layer_.get());
}

bool OffScreenRenderWidgetHostView::DelegatedFrameHostIsVisible() const {
  return !render_widget_host_->is_hidden();
}

SkColor OffScreenRenderWidgetHostView::DelegatedFrameHostGetGutterColor(
    SkColor color) const {
  if (render_widget_host_->delegate() &&
      render_widget_host_->delegate()->IsFullscreenForCurrentTab()) {
    return SK_ColorWHITE;
  }
  return color;
}

gfx::Size OffScreenRenderWidgetHostView::DelegatedFrameHostDesiredSizeInDIP()
  const {
  return GetRootLayer()->bounds().size();
}

bool OffScreenRenderWidgetHostView::DelegatedFrameCanCreateResizeLock() const {
  return !render_widget_host_->auto_resize_enabled();
}

std::unique_ptr<content::ResizeLock>
  OffScreenRenderWidgetHostView::DelegatedFrameHostCreateResizeLock(
      bool defer_compositor_lock) {
  return std::unique_ptr<content::ResizeLock>(new AtomResizeLock(
    this,
    DelegatedFrameHostDesiredSizeInDIP(),
    defer_compositor_lock));
}

void OffScreenRenderWidgetHostView::DelegatedFrameHostResizeLockWasReleased() {
  return render_widget_host_->WasResized();
}

void
OffScreenRenderWidgetHostView::DelegatedFrameHostSendReclaimCompositorResources(
    int output_surface_id,
    bool is_swap_ack,
    const cc::ReturnedResourceArray& resources) {
  render_widget_host_->Send(new ViewMsg_ReclaimCompositorResources(
      render_widget_host_->GetRoutingID(), output_surface_id, is_swap_ack,
      resources));
}

void OffScreenRenderWidgetHostView::SetBeginFrameSource(
    cc::BeginFrameSource* source) {
}

#endif  // !defined(OS_MACOSX)

bool OffScreenRenderWidgetHostView::TransformPointToLocalCoordSpace(
    const gfx::Point& point,
    const cc::SurfaceId& original_surface,
    gfx::Point* transformed_point) {
  // Transformations use physical pixels rather than DIP, so conversion
  // is necessary.
  gfx::Point point_in_pixels =
      gfx::ConvertPointToPixel(scale_factor_, point);
  if (!GetDelegatedFrameHost()->TransformPointToLocalCoordSpace(
          point_in_pixels, original_surface, transformed_point)) {
    return false;
  }

  *transformed_point =
      gfx::ConvertPointToDIP(scale_factor_, *transformed_point);
  return true;
}

bool OffScreenRenderWidgetHostView::TransformPointToCoordSpaceForView(
    const gfx::Point& point,
    RenderWidgetHostViewBase* target_view,
    gfx::Point* transformed_point) {
  if (target_view == this) {
    *transformed_point = point;
    return true;
  }

  // In TransformPointToLocalCoordSpace() there is a Point-to-Pixel conversion,
  // but it is not necessary here because the final target view is responsible
  // for converting before computing the final transform.
  return GetDelegatedFrameHost()->TransformPointToCoordSpaceForView(
      point, target_view, transformed_point);
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

void OffScreenRenderWidgetHostView::RegisterGuestViewFrameSwappedCallback(
    content::RenderWidgetHostViewGuest* guest_host_view) {
  guest_host_view->RegisterFrameSwappedCallback(base::MakeUnique<base::Closure>(
    base::Bind(&OffScreenRenderWidgetHostView::OnGuestViewFrameSwapped,
               weak_ptr_factory_.GetWeakPtr(),
               base::Unretained(guest_host_view))));
}

void OffScreenRenderWidgetHostView::OnGuestViewFrameSwapped(
    content::RenderWidgetHostViewGuest* guest_host_view) {
  InvalidateBounds(
    gfx::ConvertRectToPixel(scale_factor_, guest_host_view->GetViewBounds()));

  RegisterGuestViewFrameSwappedCallback(guest_host_view);
}

std::unique_ptr<cc::SoftwareOutputDevice>
  OffScreenRenderWidgetHostView::CreateSoftwareOutputDevice(
    ui::Compositor* compositor) {
  DCHECK_EQ(GetCompositor(), compositor);
  DCHECK(!copy_frame_generator_);
  DCHECK(!software_output_device_);

  ResizeRootLayer();

  software_output_device_ = new OffScreenOutputDevice(
      transparent_,
      base::Bind(&OffScreenRenderWidgetHostView::OnPaint,
                 weak_ptr_factory_.GetWeakPtr()));
  return base::WrapUnique(software_output_device_);
}

bool OffScreenRenderWidgetHostView::InstallTransparency() {
  if (transparent_) {
    SetBackgroundColor(SkColor());
#if defined(OS_MACOSX)
    browser_compositor_->SetHasTransparentBackground(true);
#else
    compositor_->SetHostHasTransparentBackground(true);
#endif
    return true;
  }
  return false;
}

bool OffScreenRenderWidgetHostView::IsAutoResizeEnabled() const {
  return render_widget_host_->auto_resize_enabled();
}

void OffScreenRenderWidgetHostView::SetNeedsBeginFrames(
    bool needs_begin_frames) {
  SetupFrameRate(false);

  begin_frame_timer_->SetActive(needs_begin_frames);

  if (software_output_device_) {
    software_output_device_->SetActive(needs_begin_frames && painting_);
  }
}

void CopyBitmapTo(
    const SkBitmap& destination,
    const SkBitmap& source,
    const gfx::Rect& pos) {
  SkAutoLockPixels source_pixels_lock(source);
  SkAutoLockPixels destination_pixels_lock(destination);

  char* src = static_cast<char*>(source.getPixels());
  char* dest = static_cast<char*>(destination.getPixels());
  int pixelsize = source.bytesPerPixel();

  int width = pos.x() + pos.width() <= destination.width() ? pos.width()
    : pos.width() - ((pos.x() + pos.width()) - destination.width());
  int height = pos.y() + pos.height() <= destination.height() ? pos.height()
    : pos.height() - ((pos.y() + pos.height()) - destination.height());

  if (width > 0 && height > 0) {
    for (int i = 0; i < height; i++) {
      memcpy(dest + ((pos.y() + i) * destination.width() + pos.x()) * pixelsize,
        src + (i * source.width()) * pixelsize,
        width * pixelsize);
    }
  }

  destination.notifyPixelsChanged();
}

void OffScreenRenderWidgetHostView::OnPaint(
    const gfx::Rect& damage_rect, const SkBitmap& bitmap) {
  TRACE_EVENT0("electron", "OffScreenRenderWidgetHostView::OnPaint");

  HoldResize();

  if (parent_callback_) {
    parent_callback_.Run(damage_rect, bitmap);
  } else {
    gfx::Rect damage(damage_rect);

    std::vector<gfx::Rect> damages;
    std::vector<const SkBitmap*> bitmaps;
    std::vector<SkBitmap> originals;

    if (popup_host_view_ && popup_bitmap_.get()) {
      gfx::Rect pos = popup_host_view_->popup_position_;
      damage.Union(pos);
      damages.push_back(pos);
      bitmaps.push_back(popup_bitmap_.get());
      originals.push_back(SkBitmapOperations::CreateTiledBitmap(bitmap,
        pos.x(), pos.y(), pos.width(), pos.height()));
    }

    for (auto proxy_view : proxy_views_) {
      gfx::Rect pos = proxy_view->GetBounds();
      damage.Union(pos);
      damages.push_back(pos);
      bitmaps.push_back(proxy_view->GetBitmap());
      originals.push_back(SkBitmapOperations::CreateTiledBitmap(bitmap,
        pos.x(), pos.y(), pos.width(), pos.height()));
    }

    for (size_t i = 0; i < damages.size(); i++) {
      CopyBitmapTo(bitmap, *(bitmaps[i]), damages[i]);
    }

    damage.Intersect(GetViewBounds());
    callback_.Run(damage, bitmap);

    for (size_t i = 0; i < damages.size(); i++) {
      CopyBitmapTo(bitmap, originals[i], damages[i]);
    }
  }

  ReleaseResize();
}

void OffScreenRenderWidgetHostView::OnPopupPaint(
    const gfx::Rect& damage_rect, const SkBitmap& bitmap) {
  if (popup_host_view_ && popup_bitmap_.get())
    bitmap.deepCopyTo(popup_bitmap_.get());
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
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
        base::Bind(&OffScreenRenderWidgetHostView::WasResized,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void OffScreenRenderWidgetHostView::WasResized() {
  if (hold_resize_) {
    if (!pending_resize_)
      pending_resize_ = true;
    return;
  }

  ResizeRootLayer();
  if (render_widget_host_)
    render_widget_host_->WasResized();
  GetDelegatedFrameHost()->WasResized();
}

void OffScreenRenderWidgetHostView::ProcessKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  if (!render_widget_host_)
    return;
  render_widget_host_->ForwardKeyboardEvent(event);
}

void OffScreenRenderWidgetHostView::ProcessMouseEvent(
    const blink::WebMouseEvent& event,
    const ui::LatencyInfo& latency) {
  for (auto proxy_view : proxy_views_) {
    gfx::Rect bounds = proxy_view->GetBounds();
    if (bounds.Contains(event.x, event.y)) {
      blink::WebMouseEvent proxy_event(event);
      proxy_event.x -= bounds.x();
      proxy_event.y -= bounds.y();
      proxy_event.windowX = proxy_event.x;
      proxy_event.windowY = proxy_event.y;

      ui::MouseEvent ui_event = UiMouseEventFromWebMouseEvent(proxy_event);
      proxy_view->OnEvent(&ui_event);
      return;
    }
  }

  if (!IsPopupWidget()) {
    if (popup_host_view_ &&
        popup_host_view_->popup_position_.Contains(event.x, event.y)) {
      blink::WebMouseEvent popup_event(event);
      popup_event.x -= popup_host_view_->popup_position_.x();
      popup_event.y -= popup_host_view_->popup_position_.y();
      popup_event.windowX = popup_event.x;
      popup_event.windowY = popup_event.y;

      popup_host_view_->ProcessMouseEvent(popup_event, latency);
      return;
    }
  }

  if (!render_widget_host_)
    return;
  render_widget_host_->ForwardMouseEvent(event);
}

void OffScreenRenderWidgetHostView::ProcessMouseWheelEvent(
    const blink::WebMouseWheelEvent& event,
    const ui::LatencyInfo& latency) {
  for (auto proxy_view : proxy_views_) {
    gfx::Rect bounds = proxy_view->GetBounds();
    if (bounds.Contains(event.x, event.y)) {
      blink::WebMouseWheelEvent proxy_event(event);
      proxy_event.x -= bounds.x();
      proxy_event.y -= bounds.y();
      proxy_event.windowX = proxy_event.x;
      proxy_event.windowY = proxy_event.y;

      ui::MouseWheelEvent ui_event =
        UiMouseWheelEventFromWebMouseEvent(proxy_event);
      proxy_view->OnEvent(&ui_event);
      return;
    }
  }
  if (!IsPopupWidget()) {
    if (popup_host_view_) {
      if (popup_host_view_->popup_position_.Contains(event.x, event.y)) {
        blink::WebMouseWheelEvent popup_event(event);
        popup_event.x -= popup_host_view_->popup_position_.x();
        popup_event.y -= popup_host_view_->popup_position_.y();
        popup_event.windowX = popup_event.x;
        popup_event.windowY = popup_event.y;
        popup_host_view_->ProcessMouseWheelEvent(popup_event, latency);
        return;
      } else {
        // Scrolling outside of the popup widget so destroy it.
        // Execute asynchronously to avoid deleting the widget from inside some
        // other callback.
        content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
            base::Bind(&OffScreenRenderWidgetHostView::CancelWidget,
                       popup_host_view_->weak_ptr_factory_.GetWeakPtr()));
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
    software_output_device_->SetActive(painting_);
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
    if (frame_rate > 60)
      frame_rate = 60;

    frame_rate_ = frame_rate;
  }

  for (auto guest_host_view : guest_host_views_)
    guest_host_view->SetFrameRate(frame_rate);

  SetupFrameRate(true);
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

content::DelegatedFrameHost*
OffScreenRenderWidgetHostView::GetDelegatedFrameHost() const {
  return delegated_frame_host_.get();
}
#endif

void OffScreenRenderWidgetHostView::SetupFrameRate(bool force) {
  if (!force && frame_rate_threshold_us_ != 0)
    return;

  frame_rate_threshold_us_ = 1000000 / frame_rate_;

  GetCompositor()->vsync_manager()->SetAuthoritativeVSyncInterval(
      base::TimeDelta::FromMicroseconds(frame_rate_threshold_us_));

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

void OffScreenRenderWidgetHostView::ResizeRootLayer() {
  SetupFrameRate(false);

  const float compositorScaleFactor = GetCompositor()->device_scale_factor();
  const bool scaleFactorDidChange = (compositorScaleFactor != scale_factor_);

  gfx::Size size;
  if (!IsPopupWidget())
    size = GetViewBounds().size();
  else
    size = popup_position_.size();

  if (!scaleFactorDidChange && size == GetRootLayer()->bounds().size())
    return;

  const gfx::Size& size_in_pixels =
      gfx::ConvertSizeToPixel(scale_factor_, size);

  GetRootLayer()->SetBounds(gfx::Rect(size));
  GetCompositor()->SetScaleAndSize(scale_factor_, size_in_pixels);
}

cc::FrameSinkId OffScreenRenderWidgetHostView::AllocateFrameSinkId(
    bool is_guest_view_hack) {
  // GuestViews have two RenderWidgetHostViews and so we need to make sure
  // we don't have FrameSinkId collisions.
  // The FrameSinkId generated here must be unique with FrameSinkId allocated
  // in ContextFactoryPrivate.
  content::ImageTransportFactory* factory =
      content::ImageTransportFactory::GetInstance();
  return is_guest_view_hack
          ? factory->GetContextFactoryPrivate()->AllocateFrameSinkId()
          : cc::FrameSinkId(base::checked_cast<uint32_t>(
                                render_widget_host_->GetProcess()->GetID()),
                            base::checked_cast<uint32_t>(
                                render_widget_host_->GetRoutingID()));
}

}  // namespace atom
