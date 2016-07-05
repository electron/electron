// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr_window.h"
#include "third_party/WebKit/public/platform/WebScreenInfo.h"
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

#include "content/browser/compositor/software_browser_compositor_output_surface.h"

namespace atom {

OffScreenOutputSurface::OffScreenOutputSurface(
  std::unique_ptr<cc::SoftwareOutputDevice> device)
: OutputSurface(nullptr, nullptr, std::move(device)) {
  std::cout << "OffScreenOutputSurface" << std::endl;
}

OffScreenOutputSurface::~OffScreenOutputSurface() {
  std::cout << "~OffScreenOutputSurface" << std::endl;
}

void OffScreenOutputSurface::SwapBuffers(cc::CompositorFrame* frame) {
  std::cout << "SwapBuffers" << std::endl;

  base::TimeTicks swap_time = base::TimeTicks::Now();
  for (auto& latency : frame->metadata.latency_info) {
    latency.AddLatencyNumberWithTimestamp(
      ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT, 0, 0, swap_time, 1);
    latency.AddLatencyNumberWithTimestamp(
      ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 0, 0,
      swap_time, 1);
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
    FROM_HERE, base::Bind(&content::RenderWidgetHostImpl::CompositorFrameDrawn,
      frame->metadata.latency_info));

  client_->DidSwapBuffers();
  PostSwapBuffersComplete();
}

bool OffScreenOutputSurface::BindToClient(cc::OutputSurfaceClient* client) {
  client_ = client;
  return true;
}

OffScreenOutputDevice::OffScreenOutputDevice() {
  std::cout << "OffScreenOutputDevice" << std::endl;
}

OffScreenOutputDevice::~OffScreenOutputDevice() {
  std::cout << "~OffScreenOutputDevice" << std::endl;
}

// Discards any pre-existing backing buffers and allocates memory for a
// software device of |size|. This must be called before the
// |SoftwareOutputDevice| can be used in other ways.
void OffScreenOutputDevice::Resize(
  const gfx::Size& pixel_size, float scale_factor) {
  std::cout << "Resize" << std::endl;

  std::cout << pixel_size.width() << "x" << pixel_size.height() << std::endl;

  scale_factor_ = scale_factor;
  if (viewport_pixel_size_ == pixel_size)
    return;
  viewport_pixel_size_ = pixel_size;

  surface_ = SkSurface::MakeRasterN32Premul(
    viewport_pixel_size_.width(), viewport_pixel_size_.height());
}

// Called on BeginDrawingFrame. The compositor will draw into the returned
// SkCanvas. The |SoftwareOutputDevice| implementation needs to provide a
// valid SkCanvas of at least size |damage_rect|. This class retains ownership
// of the SkCanvas.
SkCanvas* OffScreenOutputDevice::BeginPaint(const gfx::Rect& damage_rect) {
  std::cout << "BeginPaint" << std::endl;
  damage_rect_ = damage_rect;
  if (surface_.get())
    return surface_->getCanvas();

  return nullptr;
}

// Called on FinishDrawingFrame. The compositor will no longer mutate the the
// SkCanvas instance returned by |BeginPaint| and should discard any reference
// that it holds to it.
void OffScreenOutputDevice::EndPaint() {
  std::cout << "EndPaint" << std::endl;
  SkPixmap pixmap;
  if (surface_->peekPixels(&pixmap)) {
    std::cout << "OK" << std::endl;
    for(int i = 0; i < 10; i++) {
      const uint8_t* addr = reinterpret_cast<const uint8_t*>(pixmap.addr(50,i));
      for(int j = 0; j < 4; j++)
        std::cout << std::hex << static_cast<int>(*(addr + j)) << std::dec << " ";
      std::cout << std::endl;
    }
  }
}

// Discard the backing buffer in the surface provided by this instance.
void OffScreenOutputDevice::DiscardBackbuffer() {
  std::cout << "DiscardBackbuffer" << std::endl;
}

// Ensures that there is a backing buffer available on this instance.
void OffScreenOutputDevice::EnsureBackbuffer() {
  std::cout << "EnsureBackbuffer" << std::endl;
}

// VSyncProvider used to update the timer used to schedule draws with the
// hardware vsync. Return NULL if a provider doesn't exist.
gfx::VSyncProvider* OffScreenOutputDevice::GetVSyncProvider() {
  std::cout << "GetVSyncProvider" << std::endl;
  return nullptr;
}

OffScreenWindow::OffScreenWindow(
  content::RenderWidgetHost* host)
  : host_(content::RenderWidgetHostImpl::From(host)),
    delegated_frame_host_(new content::DelegatedFrameHost(this)),
    focus_(false),
    size_(gfx::Size(0,0)),
    scale_(0.0) {
  std::cout << "OffScreenWindow" << std::endl;
  //std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  host_->SetView(this);

  SetLayer(new ui::Layer(ui::LayerType::LAYER_SOLID_COLOR));
  layer()->SetVisible(true);
  layer()->set_delegate(this);
  layer()->set_name("OffScreenWindowLayer");
  layer()->SetFillsBoundsOpaquely(false);

  ui::ContextFactory* factory = content::GetContextFactory();
  thread_ = new base::Thread("Compositor");
  thread_->Start();

  //compositor_ = new ui::Compositor(factory, thread_->task_runner());
  compositor_ = new ui::Compositor(
    factory, base::ThreadTaskRunnerHandle::Get());
  delegated_frame_host_->ResetCompositor();
  delegated_frame_host_->SetCompositor(compositor_);
  compositor_->SetRootLayer(layer());

  set_layer_owner_delegate(delegated_frame_host_);

  std::unique_ptr<cc::SoftwareOutputDevice> device(new OffScreenOutputDevice());
  std::unique_ptr<cc::OutputSurface> output_surface(
    new OffScreenOutputSurface(
      std::move(device)));

  std::unique_ptr<gfx::WindowImpl> window_(new AtomCompositorHostWin());

  compositor_->SetOutputSurface(std::move(output_surface));
  compositor_->SetAcceleratedWidget(window_->hwnd());
  std::cout << compositor_ << std::endl;
}

OffScreenWindow::~OffScreenWindow() {
  std::cout << "~OffScreenWindow" << std::endl;
}

bool OffScreenWindow::OnMessageReceived(const IPC::Message& message) {
  std::cout << "OnMessageReceived" << std::endl;
  std::cout << message.type() << std::endl;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(OffScreenWindow, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_SetNeedsBeginFrames,
                        OnSetNeedsBeginFrames)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void OffScreenWindow::InitAsChild(gfx::NativeView) {
  std::cout << "InitAsChild" << std::endl;
}

content::RenderWidgetHost* OffScreenWindow::GetRenderWidgetHost() const {
  std::cout << "GetRenderWidgetHost" << std::endl;
  return host_;
}

void OffScreenWindow::SetSize(const gfx::Size& new_size) {
  std::cout << "SetSize" << std::endl;
  std::cout << new_size.width() << "x" << new_size.height() << std::endl;
  size_ = new_size;
  compositor_->SetScaleAndSize(scale_, size_);
}

void OffScreenWindow::SetBounds(const gfx::Rect& new_bounds) {
  std::cout << "SetBounds" << std::endl;
  std::cout << new_bounds.width() << "x" << new_bounds.height() << std::endl;
}

gfx::Vector2dF OffScreenWindow::GetLastScrollOffset() const {
  std::cout << "GetLastScrollOffset" << std::endl;
  return last_scroll_offset_;
}

gfx::NativeView OffScreenWindow::GetNativeView() const {
  std::cout << "GetNativeView" << std::endl;
  return static_cast<gfx::NativeView>(NULL);
}

gfx::NativeViewId OffScreenWindow::GetNativeViewId() const {
  std::cout << "GetNativeViewId" << std::endl;
  return static_cast<gfx::NativeViewId>(NULL);
}

gfx::NativeViewAccessible OffScreenWindow::GetNativeViewAccessible() {
  std::cout << "GetNativeViewAccessible" << std::endl;
  return static_cast<gfx::NativeViewAccessible>(NULL);
}

ui::TextInputClient* OffScreenWindow::GetTextInputClient() {
  std::cout << "GetTextInputClient" << std::endl;
  return nullptr;
}

void OffScreenWindow::Focus() {
  std::cout << "Focus" << std::endl;
  focus_ = true;
}

bool OffScreenWindow::HasFocus() const {
  std::cout << "HasFocus" << std::endl;
  return focus_;
}

bool OffScreenWindow::IsSurfaceAvailableForCopy() const {
  std::cout << "IsSurfaceAvailableForCopy" << std::endl;
  return delegated_frame_host_->CanCopyToBitmap();
}

void OffScreenWindow::Show() {
  std::cout << "Show" << std::endl;
  ui::LatencyInfo latency_info;

  if (delegated_frame_host_->HasSavedFrame())
    latency_info.AddLatencyNumber(
      ui::TAB_SHOW_COMPONENT, host_->GetLatencyComponentId(), 0);

  delegated_frame_host_->WasShown(latency_info);
}

void OffScreenWindow::Hide() {
  std::cout << "Hide" << std::endl;
  if (host_ && !host_->is_hidden())
    delegated_frame_host_->WasHidden();
}

bool OffScreenWindow::IsShowing() {
  std::cout << "IsShowing" << std::endl;
  return true;
}

gfx::Rect OffScreenWindow::GetViewBounds() const {
  std::cout << "GetViewBounds" << std::endl;
  return gfx::Rect(size_);
}

gfx::Size OffScreenWindow::GetVisibleViewportSize() const {
  std::cout << "GetVisibleViewportSize" << std::endl;
  return size_;
}

void OffScreenWindow::SetInsets(const gfx::Insets& insets) {
  std::cout << "SetInsets" << std::endl;
  host_->WasResized();
}

bool OffScreenWindow::LockMouse() {
  std::cout << "LockMouse" << std::endl;
  return false;
}

void OffScreenWindow::UnlockMouse() {
  std::cout << "UnlockMouse" << std::endl;
}

bool OffScreenWindow::GetScreenColorProfile(std::vector<char>*) {
  std::cout << "GetScreenColorProfile" << std::endl;
  return false;
}

void OffScreenWindow::ClearCompositorFrame() {
  std::cout << "ClearCompositorFrame" << std::endl;
  delegated_frame_host_->ClearDelegatedFrame();
}

void OffScreenWindow::InitAsPopup(content::RenderWidgetHostView *, const gfx::Rect &) {
  std::cout << "InitAsPopup" << std::endl;
}

void OffScreenWindow::InitAsFullscreen(content::RenderWidgetHostView *) {
  std::cout << "InitAsFullscreen" << std::endl;
}

void OffScreenWindow::UpdateCursor(const content::WebCursor &) {
  std::cout << "UpdateCursor" << std::endl;
}

void OffScreenWindow::SetIsLoading(bool loading) {
  std::cout << "SetIsLoading" << std::endl;
  if (!loading) {
    std::cout << "IsDrawn" << std::endl;
    std::cout << layer()->IsDrawn() << std::endl;
    layer()->SchedulePaint(gfx::Rect(size_));
  }
}

void OffScreenWindow::TextInputStateChanged(const ViewHostMsg_TextInputState_Params &) {
  std::cout << "TextInputStateChanged" << std::endl;
}

void OffScreenWindow::ImeCancelComposition() {
  std::cout << "ImeCancelComposition" << std::endl;
}

void OffScreenWindow::RenderProcessGone(base::TerminationStatus,int) {
  std::cout << "RenderProcessGone" << std::endl;
  Destroy();
}

void OffScreenWindow::Destroy() {
  std::cout << "Destroy" << std::endl;
  thread_->Stop();
}

void OffScreenWindow::SetTooltipText(const base::string16 &) {
  std::cout << "SetTooltipText" << std::endl;
}

void OffScreenWindow::SelectionBoundsChanged(const ViewHostMsg_SelectionBounds_Params &) {
  std::cout << "SelectionBoundsChanged" << std::endl;
}

void OffScreenWindow::CopyFromCompositingSurface(const gfx::Rect& src_subrect,
  const gfx::Size& dst_size,
  const content::ReadbackRequestCallback& callback,
  const SkColorType preferred_color_type) {
  delegated_frame_host_->CopyFromCompositingSurface(
    src_subrect, dst_size, callback, preferred_color_type);
  std::cout << "CopyFromCompositingSurface" << std::endl;
  delegated_frame_host_->CopyFromCompositingSurface(
    src_subrect, dst_size, callback, preferred_color_type);
}

void OffScreenWindow::CopyFromCompositingSurfaceToVideoFrame(
  const gfx::Rect& src_subrect,
  const scoped_refptr<media::VideoFrame>& target,
  const base::Callback<void (const gfx::Rect&, bool)>& callback) {
  delegated_frame_host_->CopyFromCompositingSurfaceToVideoFrame(
    src_subrect, target, callback);
  std::cout << "CopyFromCompositingSurfaceToVideoFrame" << std::endl;
  delegated_frame_host_->CopyFromCompositingSurfaceToVideoFrame(
    src_subrect, target, callback);
}

bool OffScreenWindow::CanCopyToVideoFrame() const {
  std::cout << "CanCopyToVideoFrame" << std::endl;
  return delegated_frame_host_->CanCopyToVideoFrame();
}

void OffScreenWindow::BeginFrameSubscription(
  std::unique_ptr<content::RenderWidgetHostViewFrameSubscriber> subscriber) {
  std::cout << "BeginFrameSubscription" << std::endl;
  delegated_frame_host_->BeginFrameSubscription(std::move(subscriber));
}

void OffScreenWindow::EndFrameSubscription() {
  std::cout << "EndFrameSubscription" << std::endl;
  delegated_frame_host_->EndFrameSubscription();
}

bool OffScreenWindow::HasAcceleratedSurface(const gfx::Size &) {
  std::cout << "HasAcceleratedSurface" << std::endl;
  return false;
}

void OffScreenWindow::GetScreenInfo(blink::WebScreenInfo* results) {
  std::cout << "GetScreenInfo" << std::endl;

  results->rect = gfx::Rect(size_);
  results->availableRect = gfx::Rect(size_);
  results->depth = 24;
  results->depthPerComponent = 8;
  results->deviceScaleFactor = scale_;
  results->orientationAngle = 0;
  results->orientationType = blink::WebScreenOrientationLandscapePrimary;
}

bool OffScreenWindow::GetScreenColorProfile(blink::WebVector<char>*) {
  std::cout << "GetScreenColorProfile" << std::endl;
  return false;
}

gfx::Rect OffScreenWindow::GetBoundsInRootWindow() {
  std::cout << "GetBoundsInRootWindow" << std::endl;
  return gfx::Rect(size_);
}

void OffScreenWindow::LockCompositingSurface() {
  std::cout << "LockCompositingSurface" << std::endl;
}

void OffScreenWindow::UnlockCompositingSurface() {
  std::cout << "UnlockCompositingSurface" << std::endl;
}

void OffScreenWindow::ImeCompositionRangeChanged(
  const gfx::Range &, const std::vector<gfx::Rect>&) {
  std::cout << "ImeCompositionRangeChanged" << std::endl;
}

int OffScreenWindow::DelegatedFrameHostGetGpuMemoryBufferClientId() const {
  std::cout << "DelegatedFrameHostGetGpuMemoryBufferClientId" << std::endl;
  return host_->GetProcess()->GetID();
}

ui::Layer* OffScreenWindow::DelegatedFrameHostGetLayer() const {
  std::cout << "DelegatedFrameHostGetLayer" << std::endl;
  return const_cast<ui::Layer*>(layer());
}

bool OffScreenWindow::DelegatedFrameHostIsVisible() const {
  std::cout << "DelegatedFrameHostIsVisible" << std::endl;
  std::cout << !host_->is_hidden() << std::endl;
  return !host_->is_hidden();
}

SkColor OffScreenWindow::DelegatedFrameHostGetGutterColor(SkColor color) const {
  std::cout << "DelegatedFrameHostGetGutterColor" << std::endl;
  return color;
}

gfx::Size OffScreenWindow::DelegatedFrameHostDesiredSizeInDIP() const {
  std::cout << "DelegatedFrameHostDesiredSizeInDIP" << std::endl;
  return size_;
}

bool OffScreenWindow::DelegatedFrameCanCreateResizeLock() const {
  std::cout << "DelegatedFrameCanCreateResizeLock" << std::endl;
  return false;
}

std::unique_ptr<content::ResizeLock>
  OffScreenWindow::DelegatedFrameHostCreateResizeLock(bool) {
  std::cout << "DelegatedFrameHostCreateResizeLock" << std::endl;
  return nullptr;
}

void OffScreenWindow::DelegatedFrameHostResizeLockWasReleased() {
  std::cout << "DelegatedFrameHostResizeLockWasReleased" << std::endl;
  host_->WasResized();
}

void OffScreenWindow::DelegatedFrameHostSendCompositorSwapAck(
  int output_surface_id, const cc::CompositorFrameAck& ack) {
  std::cout << "DelegatedFrameHostSendCompositorSwapAck" << std::endl;
  host_->Send(new ViewMsg_SwapCompositorFrameAck(host_->GetRoutingID(),
    output_surface_id, ack));
}

void OffScreenWindow::DelegatedFrameHostSendReclaimCompositorResources(
  int output_surface_id, const cc::CompositorFrameAck& ack) {
  std::cout << "DelegatedFrameHostSendReclaimCompositorResources" << std::endl;
  host_->Send(new ViewMsg_ReclaimCompositorResources(host_->GetRoutingID(),
    output_surface_id, ack));
}

void OffScreenWindow::DelegatedFrameHostOnLostCompositorResources() {
  std::cout << "DelegatedFrameHostOnLostCompositorResources" << std::endl;
  host_->ScheduleComposite();
}

void OffScreenWindow::DelegatedFrameHostUpdateVSyncParameters(
  const base::TimeTicks& timebase, const base::TimeDelta& interval) {
  std::cout << "DelegatedFrameHostUpdateVSyncParameters" << std::endl;
  host_->UpdateVSyncParameters(timebase, interval);
}



void OffScreenWindow::OnBeginFrame(const cc::BeginFrameArgs& args) {
  std::cout << "OnBeginFrame" << std::endl;

  delegated_frame_host_->SetVSyncParameters(args.frame_time, args.interval);
  host_->Send(new ViewMsg_BeginFrame(host_->GetRoutingID(), args));
  last_begin_frame_args_ = args;
}

const cc::BeginFrameArgs& OffScreenWindow::LastUsedBeginFrameArgs() const {
  std::cout << "LastUsedBeginFrameArgs" << std::endl;
  return last_begin_frame_args_;
}

void OffScreenWindow::OnBeginFrameSourcePausedChanged(bool) {
  std::cout << "OnBeginFrameSourcePausedChanged" << std::endl;
}

void OffScreenWindow::AsValueInto(base::trace_event::TracedValue *) const {
  std::cout << "AsValueInto" << std::endl;
}

gfx::Size OffScreenWindow::GetPhysicalBackingSize() const {
  std::cout << "GetPhysicalBackingSize" << std::endl;
  return size_;
}

void OffScreenWindow::UpdateScreenInfo(gfx::NativeView view) {
  std::cout << "UpdateScreenInfo" << std::endl;
  content::RenderWidgetHostImpl* impl = NULL;
  if (GetRenderWidgetHost())
    impl = content::RenderWidgetHostImpl::From(GetRenderWidgetHost());

  if (impl && impl->delegate())
    impl->delegate()->SendScreenRects();

  if (HasDisplayPropertyChanged(view) && impl)
    impl->NotifyScreenInfoChanged();
}

gfx::Size OffScreenWindow::GetRequestedRendererSize() const {
  std::cout << "GetRequestedRendererSize" << std::endl;
  gfx::Size size = delegated_frame_host_->GetRequestedRendererSize();

  std::cout << size.width() << "x" << size.height() << std::endl;

  return size;
}

void OffScreenWindow::OnSwapCompositorFrame(uint32_t output_surface_id,
  cc::CompositorFrame frame) {
  std::cout << "OnSwapCompositorFrame" << std::endl;

  last_scroll_offset_ = frame.metadata.root_scroll_offset;
  if (!frame.delegated_frame_data) return;

  delegated_frame_host_->SwapDelegatedFrame(
    output_surface_id, base::WrapUnique(&frame));
}

uint32_t OffScreenWindow::SurfaceIdNamespaceAtPoint(
  cc::SurfaceHittestDelegate* delegate,
  const gfx::Point& point,
  gfx::Point* transformed_point) {
  std::cout << "SurfaceIdNamespaceAtPoint" << std::endl;

  gfx::Point point_in_pixels = gfx::ConvertPointToPixel(scale_, point);
  cc::SurfaceId id = delegated_frame_host_->SurfaceIdAtPoint(
      delegate, point_in_pixels, transformed_point);
  *transformed_point = gfx::ConvertPointToDIP(scale_, *transformed_point);

  if (id.is_null())
    return GetSurfaceIdNamespace();
  return id.id_namespace();
}

uint32_t OffScreenWindow::GetSurfaceIdNamespace() {
  std::cout << "GetSurfaceIdNamespace" << std::endl;
  return delegated_frame_host_->GetSurfaceIdNamespace();
}

void OffScreenWindow::OnPaintLayer(const ui::PaintContext &) {
  std::cout << "OnPaintLayer" << std::endl;
}

void OffScreenWindow::OnDelegatedFrameDamage(const gfx::Rect &) {
  std::cout << "OnDelegatedFrameDamage" << std::endl;
}

void OffScreenWindow::OnDeviceScaleFactorChanged(float scale) {
  std::cout << "OnDeviceScaleFactorChanged" << std::endl;
  scale_ = scale;
}

base::Closure OffScreenWindow::PrepareForLayerBoundsChange() {
  std::cout << "PrepareForLayerBoundsChange" << std::endl;
  return base::Bind(&OffScreenWindow::OnBoundsChanged, base::Unretained(this));
}

void OffScreenWindow::OnBoundsChanged() {
  std::cout << "OnBoundsChanged" << std::endl;
}

void OffScreenWindow::OnSetNeedsBeginFrames(bool needs_begin_frames) {
  std::cout << "OnSetNeedsBeginFrames" << std::endl;
}

void OffScreenWindow::SendSwapCompositorFrame(cc::CompositorFrame* frame) {
  std::cout << "SendSwapCompositorFrame" << std::endl;
  std::vector<IPC::Message> messages_to_deliver_with_frame;
  /*host_->Send(new ViewHostMsg_SwapCompositorFrame(host_->GetRoutingID(),
    10, *frame, messages_to_deliver_with_frame));*/
}

}  // namespace atom
