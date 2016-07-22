// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr_window.h"
#include "third_party/WebKit/public/platform/WebScreenInfo.h"
#include "content/browser/compositor/gl_helper.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/public/browser/render_widget_host_view_frame_subscriber.h"
#include "ui/events/latency_info.h"
#include "content/common/view_messages.h"
#include "ui/gfx/geometry/dip_util.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/context_factory.h"
#include "base/single_thread_task_runner.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_type.h"
#include "base/location.h"
#include "ui/gfx/native_widget_types.h"
#include <iostream>
#include <chrono>
#include <thread>

#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "cc/output/output_surface_client.h"
#include "cc/output/copy_output_request.h"

#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "cc/scheduler/delay_based_time_source.h"

// const float kDefaultScaleFactor = 1.0;

// The maximum number of retry counts if frame capture fails.
const int kFrameRetryLimit = 2;

// When accelerated compositing is enabled and a widget resize is pending,
// we delay further resizes of the UI. The following constant is the maximum
// length of time that we should delay further UI resizes while waiting for a
// resized frame from a renderer.
// const int kResizeLockTimeoutMs = 67;

#define CEF_UIT content::BrowserThread::UI
#define CEF_POST_TASK(id, task) \
    content::BrowserThread::PostTask(id, FROM_HERE, task)
#define CEF_POST_DELAYED_TASK(id, task, delay_ms) \
    content::BrowserThread::PostDelayedTask(id, FROM_HERE, task, \
        base::TimeDelta::FromMilliseconds(delay_ms))

namespace atom {

// Used for managing copy requests when GPU compositing is enabled. Based on
// RendererOverridesHandler::InnerSwapCompositorFrame and
// DelegatedFrameHost::CopyFromCompositingSurface.
class CefCopyFrameGenerator {
 public:
  CefCopyFrameGenerator(int frame_rate_threshold_ms,
                        OffScreenWindow* view)
    : frame_rate_threshold_ms_(frame_rate_threshold_ms),
      view_(view),
      frame_pending_(false),
      frame_in_progress_(false),
      frame_retry_count_(0),
      weak_ptr_factory_(this) {
  }

  void GenerateCopyFrame(
      bool force_frame,
      const gfx::Rect& damage_rect) {
    if (force_frame && !frame_pending_)
      frame_pending_ = true;

    // No frame needs to be generated at this time.
    if (!frame_pending_)
      return;

    // Keep track of |damage_rect| for when the next frame is generated.
    if (!damage_rect.IsEmpty())
      pending_damage_rect_.Union(damage_rect);

    // Don't attempt to generate a frame while one is currently in-progress.
    if (frame_in_progress_)
      return;
    frame_in_progress_ = true;

    // Don't exceed the frame rate threshold.
    const int64_t frame_rate_delta =
        (base::TimeTicks::Now() - frame_start_time_).InMilliseconds();
    if (frame_rate_delta < frame_rate_threshold_ms_) {
      // Generate the frame after the necessary time has passed.
      CEF_POST_DELAYED_TASK(CEF_UIT,
          base::Bind(&CefCopyFrameGenerator::InternalGenerateCopyFrame,
                     weak_ptr_factory_.GetWeakPtr()),
          frame_rate_threshold_ms_ - frame_rate_delta);
      return;
    }

    InternalGenerateCopyFrame();
  }

  bool frame_pending() const { return frame_pending_; }

  void set_frame_rate_threshold_ms(int frame_rate_threshold_ms) {
    frame_rate_threshold_ms_ = frame_rate_threshold_ms;
  }

 private:
  void InternalGenerateCopyFrame() {
    frame_pending_ = false;
    frame_start_time_ = base::TimeTicks::Now();

    if (!view_->render_widget_host())
      return;

    const gfx::Rect damage_rect = pending_damage_rect_;
    pending_damage_rect_.SetRect(0, 0, 0, 0);

    // The below code is similar in functionality to
    // DelegatedFrameHost::CopyFromCompositingSurface but we reuse the same
    // SkBitmap in the GPU codepath and avoid scaling where possible.
    std::unique_ptr<cc::CopyOutputRequest> request =
        cc::CopyOutputRequest::CreateRequest(base::Bind(
            &CefCopyFrameGenerator::CopyFromCompositingSurfaceHasResult,
            weak_ptr_factory_.GetWeakPtr(),
            damage_rect));

    request->set_area(gfx::Rect(view_->GetPhysicalBackingSize()));

    view_->DelegatedFrameHostGetLayer()->RequestCopyOfOutput(
        std::move(request));
  }

  void CopyFromCompositingSurfaceHasResult(
      const gfx::Rect& damage_rect,
      std::unique_ptr<cc::CopyOutputResult> result) {
    // std::cout << "has result" << std::endl;
    if (result->IsEmpty() || result->size().IsEmpty() ||
        !view_->render_widget_host()) {
      OnCopyFrameCaptureFailure(damage_rect);
      return;
    }

    if (result->HasTexture()) {
      PrepareTextureCopyOutputResult(damage_rect, std::move(result));
      return;
    }

    DCHECK(result->HasBitmap());
    PrepareBitmapCopyOutputResult(damage_rect, std::move(result));
  }

  void PrepareTextureCopyOutputResult(
      const gfx::Rect& damage_rect,
      std::unique_ptr<cc::CopyOutputResult> result) {
    DCHECK(result->HasTexture());
    base::ScopedClosureRunner scoped_callback_runner(
        base::Bind(&CefCopyFrameGenerator::OnCopyFrameCaptureFailure,
                   weak_ptr_factory_.GetWeakPtr(),
                   damage_rect));

    const gfx::Size& result_size = result->size();
    SkIRect bitmap_size;
    if (bitmap_)
      bitmap_->getBounds(&bitmap_size);

    if (!bitmap_ ||
        bitmap_size.width() != result_size.width() ||
        bitmap_size.height() != result_size.height()) {
      // Create a new bitmap if the size has changed.
      bitmap_.reset(new SkBitmap);
      bitmap_->allocN32Pixels(result_size.width(),
                              result_size.height(),
                              true);
      if (bitmap_->drawsNothing())
        return;
    }

    content::ImageTransportFactory* factory =
        content::ImageTransportFactory::GetInstance();
    content::GLHelper* gl_helper = factory->GetGLHelper();
    if (!gl_helper)
      return;

    std::unique_ptr<SkAutoLockPixels> bitmap_pixels_lock(
        new SkAutoLockPixels(*bitmap_));
    uint8_t* pixels = static_cast<uint8_t*>(bitmap_->getPixels());

    cc::TextureMailbox texture_mailbox;
    std::unique_ptr<cc::SingleReleaseCallback> release_callback;
    result->TakeTexture(&texture_mailbox, &release_callback);
    DCHECK(texture_mailbox.IsTexture());
    if (!texture_mailbox.IsTexture())
      return;

    ignore_result(scoped_callback_runner.Release());

    gl_helper->CropScaleReadbackAndCleanMailbox(
        texture_mailbox.mailbox(),
        texture_mailbox.sync_token(),
        result_size,
        gfx::Rect(result_size),
        result_size,
        pixels,
        kN32_SkColorType,
        base::Bind(
            &CefCopyFrameGenerator::CopyFromCompositingSurfaceFinishedProxy,
            weak_ptr_factory_.GetWeakPtr(),
            base::Passed(&release_callback),
            damage_rect,
            base::Passed(&bitmap_),
            base::Passed(&bitmap_pixels_lock)),
        content::GLHelper::SCALER_QUALITY_FAST);
  }

  static void CopyFromCompositingSurfaceFinishedProxy(
      base::WeakPtr<CefCopyFrameGenerator> generator,
      std::unique_ptr<cc::SingleReleaseCallback> release_callback,
      const gfx::Rect& damage_rect,
      std::unique_ptr<SkBitmap> bitmap,
      std::unique_ptr<SkAutoLockPixels> bitmap_pixels_lock,
      bool result) {
    // This method may be called after the view has been deleted.
    gpu::SyncToken sync_token;
    if (result) {
      content::GLHelper* gl_helper =
          content::ImageTransportFactory::GetInstance()->GetGLHelper();
      if (gl_helper)
        gl_helper->GenerateSyncToken(&sync_token);
    }
    const bool lost_resource = !sync_token.HasData();
    release_callback->Run(sync_token, lost_resource);

    if (generator) {
      generator->CopyFromCompositingSurfaceFinished(
          damage_rect, std::move(bitmap), std::move(bitmap_pixels_lock),
          result);
    } else {
      bitmap_pixels_lock.reset();
      bitmap.reset();
    }
  }

  void CopyFromCompositingSurfaceFinished(
      const gfx::Rect& damage_rect,
      std::unique_ptr<SkBitmap> bitmap,
      std::unique_ptr<SkAutoLockPixels> bitmap_pixels_lock,
      bool result) {
    // Restore ownership of the bitmap to the view.
    DCHECK(!bitmap_);
    bitmap_ = std::move(bitmap);

    if (result) {
      OnCopyFrameCaptureSuccess(damage_rect, *bitmap_,
                                std::move(bitmap_pixels_lock));
    } else {
      bitmap_pixels_lock.reset();
      OnCopyFrameCaptureFailure(damage_rect);
    }
  }

  void PrepareBitmapCopyOutputResult(
      const gfx::Rect& damage_rect,
      std::unique_ptr<cc::CopyOutputResult> result) {
    DCHECK(result->HasBitmap());
    std::unique_ptr<SkBitmap> source = result->TakeBitmap();
    DCHECK(source);
    if (source) {
      std::unique_ptr<SkAutoLockPixels> bitmap_pixels_lock(
          new SkAutoLockPixels(*source));
      OnCopyFrameCaptureSuccess(damage_rect, *source,
                                std::move(bitmap_pixels_lock));
    } else {
      OnCopyFrameCaptureFailure(damage_rect);
    }
  }

  void OnCopyFrameCaptureFailure(
      const gfx::Rect& damage_rect) {
    // Retry with the same |damage_rect|.
    pending_damage_rect_.Union(damage_rect);

    const bool force_frame = (++frame_retry_count_ <= kFrameRetryLimit);
    OnCopyFrameCaptureCompletion(force_frame);
  }

  void OnCopyFrameCaptureSuccess(
      const gfx::Rect& damage_rect,
      const SkBitmap& bitmap,
      std::unique_ptr<SkAutoLockPixels> bitmap_pixels_lock) {

    uint8_t* pixels = reinterpret_cast<uint8_t*>(bitmap.getPixels());
    // for (int i = 0; i<4; i++) {
    //   int x = static_cast<int>(pixels[i]);
    // std::cout << std::hex << x << std::dec << std::endl;
    // }
    if (view_->paintCallback) {
      view_->paintCallback->Run(damage_rect, bitmap.width(), bitmap.height(),
                pixels);
    }

    bitmap_pixels_lock.reset();

    // Reset the frame retry count on successful frame generation.
    if (frame_retry_count_ > 0)
      frame_retry_count_ = 0;

    OnCopyFrameCaptureCompletion(false);
  }

  void OnCopyFrameCaptureCompletion(bool force_frame) {
    frame_in_progress_ = false;

    if (frame_pending_) {
      // Another frame was requested while the current frame was in-progress.
      // Generate the pending frame now.
      CEF_POST_TASK(CEF_UIT,
          base::Bind(&CefCopyFrameGenerator::GenerateCopyFrame,
                     weak_ptr_factory_.GetWeakPtr(),
                     force_frame,
                     gfx::Rect()));
    }
  }

  int frame_rate_threshold_ms_;
  OffScreenWindow* view_;

  base::TimeTicks frame_start_time_;
  bool frame_pending_;
  bool frame_in_progress_;
  int frame_retry_count_;
  std::unique_ptr<SkBitmap> bitmap_;
  gfx::Rect pending_damage_rect_;

  base::WeakPtrFactory<CefCopyFrameGenerator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CefCopyFrameGenerator);
};

// Used to control the VSync rate in subprocesses when BeginFrame scheduling is
// enabled.
class CefBeginFrameTimer : public cc::DelayBasedTimeSourceClient {
 public:
  CefBeginFrameTimer(int frame_rate_threshold_ms,
                     const base::Closure& callback)
      : callback_(callback) {
    time_source_ = cc::DelayBasedTimeSource::Create(
        base::TimeDelta::FromMilliseconds(frame_rate_threshold_ms),
        content::BrowserThread::GetMessageLoopProxyForThread(CEF_UIT).get());
    time_source_->SetClient(this);
  }

  void SetActive(bool active) {
    time_source_->SetActive(active);
  }

  bool IsActive() const {
    return time_source_->Active();
  }

  void SetFrameRateThresholdMs(int frame_rate_threshold_ms) {
    time_source_->SetTimebaseAndInterval(
        base::TimeTicks::Now(),
        base::TimeDelta::FromMilliseconds(frame_rate_threshold_ms));
  }

 private:
  // cc::TimerSourceClient implementation.
  void OnTimerTick() override {
    callback_.Run();
  }

  const base::Closure callback_;
  std::unique_ptr<cc::DelayBasedTimeSource> time_source_;

  DISALLOW_COPY_AND_ASSIGN(CefBeginFrameTimer);
};

OffScreenWindow::OffScreenWindow(content::RenderWidgetHost* host)
  : render_widget_host_(content::RenderWidgetHostImpl::From(host)),
    software_output_device_(NULL),
    frame_rate_threshold_ms_(0),
    scale_factor_(1.0f),
    is_showing_(!render_widget_host_->is_hidden()),
    size_(gfx::Size(800, 600)),
    delegated_frame_host_(new content::DelegatedFrameHost(this)),
    compositor_widget_(gfx::kNullAcceleratedWidget),
    weak_ptr_factory_(this) {
  DCHECK(render_widget_host_);
  // std::cout << "OffScreenWindow" << std::endl;
  render_widget_host_->SetView(this);

  root_layer_.reset(new ui::Layer(ui::LAYER_SOLID_COLOR));

  CreatePlatformWidget();

#if !defined(OS_MACOSX)
  // On OS X the ui::Compositor is created/owned by the platform view.
  compositor_.reset(
      new ui::Compositor(content::GetContextFactory(),
                         base::ThreadTaskRunnerHandle::Get()));
  compositor_->SetAcceleratedWidget(compositor_widget_);
#endif
  // compositor_->SetDelegate(this);
  compositor_->SetRootLayer(root_layer_.get());
}

void OffScreenWindow::ResizeRootLayer() {
  SetFrameRate();

  // const float orgScaleFactor = scale_factor_;
  // SetDeviceScaleFactor();
  // const bool scaleFactorDidChange = (orgScaleFactor != scale_factor_);
  //
  // gfx::Size size;
  // if (!IsPopupWidget())
  //   size = GetViewBounds().size();
  // else
  //   size = popup_position_.size();
  //
  // if (!scaleFactorDidChange && size == root_layer_->bounds().size())
  //   return;
  //
  // const gfx::Size& size_in_pixels =
  //     gfx::ConvertSizeToPixel(scale_factor_, size);
  //
  // root_layer_->SetBounds(gfx::Rect(size));
  // compositor_->SetScaleAndSize(scale_factor_, size_in_pixels);
}

void OffScreenWindow::OnBeginFrameTimerTick() {
  const base::TimeTicks frame_time = base::TimeTicks::Now();
  const base::TimeDelta vsync_period =
      base::TimeDelta::FromMilliseconds(frame_rate_threshold_ms_);
  SendBeginFrame(frame_time, vsync_period);
}

void OffScreenWindow::SendBeginFrame(base::TimeTicks frame_time,
                                                base::TimeDelta vsync_period) {
  base::TimeTicks display_time = frame_time + vsync_period;

  // TODO(brianderson): Use adaptive draw-time estimation.
  base::TimeDelta estimated_browser_composite_time =
      base::TimeDelta::FromMicroseconds(
          (1.0f * base::Time::kMicrosecondsPerSecond) / (3.0f * 60));

  base::TimeTicks deadline = display_time - estimated_browser_composite_time;

  render_widget_host_->Send(new ViewMsg_BeginFrame(
      render_widget_host_->GetRoutingID(),
      cc::BeginFrameArgs::Create(BEGINFRAME_FROM_HERE, frame_time, deadline,
                                 vsync_period, cc::BeginFrameArgs::NORMAL)));
  // std::cout << "sent begin frame" << std::endl;
}

void OffScreenWindow::SetPaintCallback(const OnPaintCallback *callback) {
  paintCallback.reset(callback);
}

OffScreenWindow::~OffScreenWindow() {
  // std::cout << "~OffScreenWindow" << std::endl;

  if (is_showing_)
    delegated_frame_host_->WasHidden();
  delegated_frame_host_->ResetCompositor();

  delegated_frame_host_.reset(NULL);
  compositor_.reset(NULL);
  root_layer_.reset(NULL);
}

bool OffScreenWindow::OnMessageReceived(const IPC::Message& message) {
  // std::cout << "OnMessageReceived" << std::endl;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(OffScreenWindow, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_SetNeedsBeginFrames,
                        OnSetNeedsBeginFrames)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  if (!handled)
    return content::RenderWidgetHostViewBase::OnMessageReceived(message);
  return handled;
}

void OffScreenWindow::InitAsChild(gfx::NativeView) {
  // std::cout << "InitAsChild" << std::endl;
}

content::RenderWidgetHost* OffScreenWindow::GetRenderWidgetHost() const {
  // std::cout << "GetRenderWidgetHost" << std::endl;
  return render_widget_host_;
}

void OffScreenWindow::SetSize(const gfx::Size& size) {
  // std::cout << "SetSize" << std::endl;
  size_ = size;

  // std::cout << size.width() << "x" << size.height() << std::endl;

  const gfx::Size& size_in_pixels =
    gfx::ConvertSizeToPixel(scale_factor_, size);

  root_layer_->SetBounds(gfx::Rect(size));
  compositor_->SetScaleAndSize(scale_factor_, size_in_pixels);
}

void OffScreenWindow::SetBounds(const gfx::Rect& new_bounds) {
  // std::cout << "SetBounds" << std::endl;
}

gfx::Vector2dF OffScreenWindow::GetLastScrollOffset() const {
  // std::cout << "GetLastScrollOffset" << std::endl;
  return last_scroll_offset_;
}

gfx::NativeView OffScreenWindow::GetNativeView() const {
  // std::cout << "GetNativeView" << std::endl;
  return gfx::NativeView();
}

gfx::NativeViewId OffScreenWindow::GetNativeViewId() const {
  // std::cout << "GetNativeViewId" << std::endl;
  return gfx::NativeViewId();
}

gfx::NativeViewAccessible OffScreenWindow::GetNativeViewAccessible() {
  // std::cout << "GetNativeViewAccessible" << std::endl;
  return gfx::NativeViewAccessible();
}

ui::TextInputClient* OffScreenWindow::GetTextInputClient() {
  // std::cout << "GetTextInputClient" << std::endl;
  return nullptr;
}

void OffScreenWindow::Focus() {
  // std::cout << "Focus" << std::endl;
}

bool OffScreenWindow::HasFocus() const {
  // std::cout << "HasFocus" << std::endl;
  return false;
}

bool OffScreenWindow::IsSurfaceAvailableForCopy() const {
  // std::cout << "IsSurfaceAvailableForCopy" << std::endl;
  return delegated_frame_host_->CanCopyToBitmap();
}

void OffScreenWindow::Show() {
  // std::cout << "Show" << std::endl;
  if (is_showing_)
    return;

  is_showing_ = true;
  if (render_widget_host_)
    render_widget_host_->WasShown(ui::LatencyInfo());
  delegated_frame_host_->SetCompositor(compositor_.get());
  delegated_frame_host_->WasShown(ui::LatencyInfo());
}

void OffScreenWindow::Hide() {
  // std::cout << "Hide" << std::endl;
  if (!is_showing_)
    return;

  if (render_widget_host_)
    render_widget_host_->WasHidden();
  delegated_frame_host_->WasHidden();
  delegated_frame_host_->ResetCompositor();
  is_showing_ = false;
}

bool OffScreenWindow::IsShowing() {
  // std::cout << "IsShowing" << std::endl;
  return is_showing_;
}

gfx::Rect OffScreenWindow::GetViewBounds() const {
  // std::cout << "GetViewBounds" << std::endl;
  // std::cout << size_.width() << "x" << size_.height() << std::endl;
  return gfx::Rect(size_);
}

gfx::Size OffScreenWindow::GetVisibleViewportSize() const {
  // std::cout << "GetVisibleViewportSize" << std::endl;
  // std::cout << size_.width() << "x" << size_.height() << std::endl;
  return size_;
}

void OffScreenWindow::SetInsets(const gfx::Insets& insets) {
  // std::cout << "SetInsets" << std::endl;
}

bool OffScreenWindow::LockMouse() {
  // std::cout << "LockMouse" << std::endl;
  return false;
}

void OffScreenWindow::UnlockMouse() {
  // std::cout << "UnlockMouse" << std::endl;
}

bool OffScreenWindow::GetScreenColorProfile(std::vector<char>*) {
  // std::cout << "GetScreenColorProfile" << std::endl;
  return false;
}

void OffScreenWindow::OnSwapCompositorFrame(
  uint32_t output_surface_id,
  std::unique_ptr<cc::CompositorFrame> frame) {
  // std::cout << "OnSwapCompositorFrame" << std::endl;

  // std::cout << output_surface_id << std::endl;

  if (frame->metadata.root_scroll_offset != last_scroll_offset_) {
    last_scroll_offset_ = frame->metadata.root_scroll_offset;
  }

  if (!frame->delegated_frame_data) {
    return;
  }

  if (software_output_device_) {
    // if (!begin_frame_timer_.get()) {
    //   software_output_device_->SetActive(true);
    // }

    delegated_frame_host_->SwapDelegatedFrame(output_surface_id,
                                              std::move(frame));
    return;
  }

  if (!copy_frame_generator_.get()) {
    copy_frame_generator_.reset(
        new CefCopyFrameGenerator(frame_rate_threshold_ms_, this));
  }

  // Determine the damage rectangle for the current frame. This is the same
  // calculation that SwapDelegatedFrame uses.
  cc::RenderPass* root_pass =
      frame->delegated_frame_data->render_pass_list.back().get();
  gfx::Size frame_size = root_pass->output_rect.size();
  gfx::Rect damage_rect =
      gfx::ToEnclosingRect(gfx::RectF(root_pass->damage_rect));
  damage_rect.Intersect(gfx::Rect(frame_size));

  if (frame->delegated_frame_data)
    delegated_frame_host_->SwapDelegatedFrame(output_surface_id,
                                                std::move(frame));

  // Request a copy of the last compositor frame which will eventually call
  // OnPaint asynchronously.
  // std::cout << "FRAME COPY REQUESTED" << std::endl;
  copy_frame_generator_->GenerateCopyFrame(true, damage_rect);
}

void OffScreenWindow::ClearCompositorFrame() {
  // std::cout << "ClearCompositorFrame" << std::endl;
  delegated_frame_host_->ClearDelegatedFrame();
}

void OffScreenWindow::InitAsPopup(
  content::RenderWidgetHostView *, const gfx::Rect &) {
  // std::cout << "InitAsPopup" << std::endl;
}

void OffScreenWindow::InitAsFullscreen(content::RenderWidgetHostView *) {
  // std::cout << "InitAsFullscreen" << std::endl;
}

void OffScreenWindow::UpdateCursor(const content::WebCursor &) {
  // std::cout << "UpdateCursor" << std::endl;
}

void OffScreenWindow::SetIsLoading(bool loading) {
  // std::cout << "SetIsLoading" << std::endl;
}

void OffScreenWindow::TextInputStateChanged(
  const ViewHostMsg_TextInputState_Params &) {
  // std::cout << "TextInputStateChanged" << std::endl;
}

void OffScreenWindow::ImeCancelComposition() {
  // std::cout << "ImeCancelComposition" << std::endl;
}

void OffScreenWindow::RenderProcessGone(base::TerminationStatus,int) {
  // std::cout << "RenderProcessGone" << std::endl;
  Destroy();
}

void OffScreenWindow::Destroy() {
  // std::cout << "Destroy" << std::endl;
}

void OffScreenWindow::SetTooltipText(const base::string16 &) {
  // std::cout << "SetTooltipText" << std::endl;
}

void OffScreenWindow::SelectionBoundsChanged(
  const ViewHostMsg_SelectionBounds_Params &) {
  // std::cout << "SelectionBoundsChanged" << std::endl;
}

void OffScreenWindow::CopyFromCompositingSurface(const gfx::Rect& src_subrect,
  const gfx::Size& dst_size,
  const content::ReadbackRequestCallback& callback,
  const SkColorType preferred_color_type) {
  // std::cout << "CopyFromCompositingSurface" << std::endl;
  delegated_frame_host_->CopyFromCompositingSurface(
    src_subrect, dst_size, callback, preferred_color_type);
}

void OffScreenWindow::CopyFromCompositingSurfaceToVideoFrame(
  const gfx::Rect& src_subrect,
  const scoped_refptr<media::VideoFrame>& target,
  const base::Callback<void (const gfx::Rect&, bool)>& callback) {
  // std::cout << "CopyFromCompositingSurfaceToVideoFrame" << std::endl;
  delegated_frame_host_->CopyFromCompositingSurfaceToVideoFrame(
    src_subrect, target, callback);
}

bool OffScreenWindow::CanCopyToVideoFrame() const {
  // std::cout << "CanCopyToVideoFrame" << std::endl;
  return delegated_frame_host_->CanCopyToVideoFrame();
}

void OffScreenWindow::BeginFrameSubscription(
  std::unique_ptr<content::RenderWidgetHostViewFrameSubscriber> subscriber) {
  // std::cout << "BeginFrameSubscription" << std::endl;
  delegated_frame_host_->BeginFrameSubscription(std::move(subscriber));
}

void OffScreenWindow::EndFrameSubscription() {
  // std::cout << "EndFrameSubscription" << std::endl;
  delegated_frame_host_->EndFrameSubscription();
}

bool OffScreenWindow::HasAcceleratedSurface(const gfx::Size &) {
  // std::cout << "HasAcceleratedSurface" << std::endl;
  return false;
}

void OffScreenWindow::GetScreenInfo(blink::WebScreenInfo* results) {
  // std::cout << "GetScreenInfo" << std::endl;
  // std::cout << size_.width() << "x" << size_.height() << std::endl;
  results->rect = gfx::Rect(size_);
  results->availableRect = gfx::Rect(size_);
  results->depth = 24;
  results->depthPerComponent = 8;
  results->deviceScaleFactor = scale_factor_;
  results->orientationAngle = 0;
  results->orientationType = blink::WebScreenOrientationLandscapePrimary;
}

bool OffScreenWindow::GetScreenColorProfile(blink::WebVector<char>*) {
  // std::cout << "GetScreenColorProfile" << std::endl;
  return false;
}

gfx::Rect OffScreenWindow::GetBoundsInRootWindow() {
  // std::cout << "GetBoundsInRootWindow" << std::endl;
  // std::cout << size_.width() << "x" << size_.height() << std::endl;
  return gfx::Rect(size_);
}

void OffScreenWindow::LockCompositingSurface() {
  // std::cout << "LockCompositingSurface" << std::endl;
}

void OffScreenWindow::UnlockCompositingSurface() {
  // std::cout << "UnlockCompositingSurface" << std::endl;
}

void OffScreenWindow::ImeCompositionRangeChanged(
  const gfx::Range &, const std::vector<gfx::Rect>&) {
  // std::cout << "ImeCompositionRangeChanged" << std::endl;
}

gfx::Size OffScreenWindow::GetPhysicalBackingSize() const {
  // std::cout << "GetPhysicalBackingSize" << std::endl;
  return size_;
}

gfx::Size OffScreenWindow::GetRequestedRendererSize() const {
  // std::cout << "GetRequestedRendererSize" << std::endl;
  return size_;
}

int OffScreenWindow::DelegatedFrameHostGetGpuMemoryBufferClientId() const {
  // std::cout << "DelegatedFrameHostGetGpuMemoryBufferClientId" << std::endl;
  return render_widget_host_->GetProcess()->GetID();
}

ui::Layer* OffScreenWindow::DelegatedFrameHostGetLayer() const {
  // std::cout << "DelegatedFrameHostGetLayer" << std::endl;
  return const_cast<ui::Layer*>(root_layer_.get());
}

bool OffScreenWindow::DelegatedFrameHostIsVisible() const {
  // std::cout << "DelegatedFrameHostIsVisible" << std::endl;
  // std::cout << !render_widget_host_->is_hidden() << std::endl;
  return !render_widget_host_->is_hidden();
}

SkColor OffScreenWindow::DelegatedFrameHostGetGutterColor(SkColor color) const {
  // std::cout << "DelegatedFrameHostGetGutterColor" << std::endl;
  return color;
}

gfx::Size OffScreenWindow::DelegatedFrameHostDesiredSizeInDIP() const {
  // std::cout << "DelegatedFrameHostDesiredSizeInDIP" << std::endl;
  // std::cout << size_.width() << "x" << size_.height() << std::endl;
  return size_;
}

bool OffScreenWindow::DelegatedFrameCanCreateResizeLock() const {
  // std::cout << "DelegatedFrameCanCreateResizeLock" << std::endl;
  return false;
}

std::unique_ptr<content::ResizeLock>
  OffScreenWindow::DelegatedFrameHostCreateResizeLock(bool) {
  // std::cout << "DelegatedFrameHostCreateResizeLock" << std::endl;
  return nullptr;
}

void OffScreenWindow::DelegatedFrameHostResizeLockWasReleased() {
  // std::cout << "DelegatedFrameHostResizeLockWasReleased" << std::endl;
  return render_widget_host_->WasResized();
}

void OffScreenWindow::DelegatedFrameHostSendCompositorSwapAck(
  int output_surface_id, const cc::CompositorFrameAck& ack) {
  // std::cout << "DelegatedFrameHostSendCompositorSwapAck" << std::endl;
  // std::cout << output_surface_id << std::endl;
  render_widget_host_->Send(new ViewMsg_SwapCompositorFrameAck(
    render_widget_host_->GetRoutingID(),
    output_surface_id, ack));
}

void OffScreenWindow::DelegatedFrameHostSendReclaimCompositorResources(
  int output_surface_id, const cc::CompositorFrameAck& ack) {
  // std::cout << "DelegatedFrameHostSendReclaimCompositorResources" << std::endl;
  // std::cout << output_surface_id << std::endl;
  render_widget_host_->Send(new ViewMsg_ReclaimCompositorResources(
    render_widget_host_->GetRoutingID(),
    output_surface_id, ack));
}

void OffScreenWindow::DelegatedFrameHostOnLostCompositorResources() {
  // std::cout << "DelegatedFrameHostOnLostCompositorResources" << std::endl;
  render_widget_host_->ScheduleComposite();
}

void OffScreenWindow::DelegatedFrameHostUpdateVSyncParameters(
  const base::TimeTicks& timebase, const base::TimeDelta& interval) {
  // std::cout << "DelegatedFrameHostUpdateVSyncParameters" << std::endl;
  render_widget_host_->UpdateVSyncParameters(timebase, interval);
}

std::unique_ptr<cc::SoftwareOutputDevice>
OffScreenWindow::CreateSoftwareOutputDevice(
    ui::Compositor* compositor) {
  DCHECK_EQ(compositor_.get(), compositor);
  DCHECK(!copy_frame_generator_);
  DCHECK(!software_output_device_);
  std::cout << "CREATED" << std::endl;
  software_output_device_ = new OffScreenOutputDevice();
  return base::WrapUnique(software_output_device_);
}

void OffScreenWindow::OnSetNeedsBeginFrames(bool enabled) {
  SetFrameRate();

  begin_frame_timer_->SetActive(enabled);

  // std::cout << "OnSetNeedsBeginFrames" << enabled << std::endl;
}

void OffScreenWindow::SetFrameRate() {
  // Only set the frame rate one time.
  if (frame_rate_threshold_ms_ != 0)
    return;

  const int frame_rate = 120;
  frame_rate_threshold_ms_ = 1000 / frame_rate;

  // Configure the VSync interval for the browser process.
  compositor_->vsync_manager()->SetAuthoritativeVSyncInterval(
      base::TimeDelta::FromMilliseconds(frame_rate_threshold_ms_));

  if (copy_frame_generator_.get()) {
    copy_frame_generator_->set_frame_rate_threshold_ms(
        frame_rate_threshold_ms_);
  }

  if (begin_frame_timer_.get()) {
    begin_frame_timer_->SetFrameRateThresholdMs(frame_rate_threshold_ms_);
  } else {
    begin_frame_timer_.reset(new CefBeginFrameTimer(
        frame_rate_threshold_ms_,
        base::Bind(&OffScreenWindow::OnBeginFrameTimerTick,
                   weak_ptr_factory_.GetWeakPtr())));
  }
}

}  // namespace atom
