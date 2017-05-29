// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_
#define ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_

#include <set>
#include <string>
#include <vector>

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "atom/browser/native_window.h"
#include "atom/browser/native_window_observer.h"
#include "atom/browser/osr/osr_output_device.h"
#include "atom/browser/osr/osr_view_proxy.h"
#include "base/process/kill.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "cc/output/compositor_frame.h"
#include "cc/scheduler/begin_frame_source.h"
#include "content/browser/frame_host/render_widget_host_view_guest.h"
#include "content/browser/renderer_host/delegated_frame_host.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/renderer_host/resize_lock.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/gfx/geometry/point.h"

#if defined(OS_WIN)
#include "ui/gfx/win/window_impl.h"
#endif

#if defined(OS_MACOSX)
#include "content/browser/renderer_host/browser_compositor_view_mac.h"
#endif

#if defined(OS_MACOSX)
#ifdef __OBJC__
@class CALayer;
@class NSWindow;
#else
class CALayer;
class NSWindow;
#endif
#endif

namespace atom {

class AtomCopyFrameGenerator;
class AtomBeginFrameTimer;

#if defined(OS_MACOSX)
class MacHelper;
#endif

class OffScreenRenderWidgetHostView
    : public content::RenderWidgetHostViewBase,
      public ui::CompositorDelegate,
#if !defined(OS_MACOSX)
      public content::DelegatedFrameHostClient,
#endif
      public NativeWindowObserver,
      public OffscreenViewProxyObserver {
 public:
  OffScreenRenderWidgetHostView(bool transparent,
                                const OnPaintCallback& callback,
                                content::RenderWidgetHost* render_widget_host,
                                OffScreenRenderWidgetHostView* parent_host_view,
                                NativeWindow* native_window);
  ~OffScreenRenderWidgetHostView() override;

  // content::RenderWidgetHostView:
  bool OnMessageReceived(const IPC::Message&) override;
  void InitAsChild(gfx::NativeView) override;
  content::RenderWidgetHost* GetRenderWidgetHost(void) const override;
  void SetSize(const gfx::Size &) override;
  void SetBounds(const gfx::Rect &) override;
  gfx::Vector2dF GetLastScrollOffset(void) const override;
  gfx::NativeView GetNativeView(void) const override;
  gfx::NativeViewAccessible GetNativeViewAccessible(void) override;
  ui::TextInputClient* GetTextInputClient() override;
  void Focus(void) override;
  bool HasFocus(void) const override;
  bool IsSurfaceAvailableForCopy(void) const override;
  void Show(void) override;
  void Hide(void) override;
  bool IsShowing(void) override;
  gfx::Rect GetViewBounds(void) const override;
  gfx::Size GetVisibleViewportSize() const override;
  void SetInsets(const gfx::Insets&) override;
  void SetBackgroundColor(SkColor color) override;
  bool LockMouse(void) override;
  void UnlockMouse(void) override;
  void SetNeedsBeginFrames(bool needs_begin_frames) override;
#if defined(OS_MACOSX)
  ui::AcceleratedWidgetMac* GetAcceleratedWidgetMac() const override;
  void SetActive(bool active) override;
  void ShowDefinitionForSelection() override;
  bool SupportsSpeech() const override;
  void SpeakSelection() override;
  bool IsSpeaking() const override;
  void StopSpeaking() override;
#endif  // defined(OS_MACOSX)

  // content::RenderWidgetHostViewBase:
  void OnSwapCompositorFrame(uint32_t, cc::CompositorFrame)
    override;
  void ClearCompositorFrame(void) override;
  void InitAsPopup(content::RenderWidgetHostView *rwhv, const gfx::Rect& rect)
    override;
  void InitAsFullscreen(content::RenderWidgetHostView *) override;
  void UpdateCursor(const content::WebCursor &) override;
  void SetIsLoading(bool is_loading) override;
  void TextInputStateChanged(const content::TextInputState& params) override;
  void ImeCancelComposition(void) override;
  void RenderProcessGone(base::TerminationStatus, int) override;
  void Destroy(void) override;
  void SetTooltipText(const base::string16 &) override;
#if defined(OS_MACOSX)
  void SelectionChanged(const base::string16& text,
                        size_t offset,
                        const gfx::Range& range) override;
#endif
  void SelectionBoundsChanged(const ViewHostMsg_SelectionBounds_Params &)
    override;
  void CopyFromSurface(
      const gfx::Rect& src_subrect,
      const gfx::Size& dst_size,
      const content::ReadbackRequestCallback& callback,
      const SkColorType color_type) override;
  void CopyFromSurfaceToVideoFrame(
      const gfx::Rect& src_subrect,
      scoped_refptr<media::VideoFrame> target,
      const base::Callback<void(const gfx::Rect&, bool)>& callback) override;
  void BeginFrameSubscription(
    std::unique_ptr<content::RenderWidgetHostViewFrameSubscriber>) override;
  void EndFrameSubscription() override;
  void InitAsGuest(
    content::RenderWidgetHostView*,
    content::RenderWidgetHostViewGuest*) override;
  bool HasAcceleratedSurface(const gfx::Size &) override;
  gfx::Rect GetBoundsInRootWindow(void) override;
  void ImeCompositionRangeChanged(
    const gfx::Range &, const std::vector<gfx::Rect>&) override;
  gfx::Size GetPhysicalBackingSize() const override;
  gfx::Size GetRequestedRendererSize() const override;

  content::RenderWidgetHostViewBase* CreateViewForWidget(
    content::RenderWidgetHost*,
    content::RenderWidgetHost*,
    content::WebContentsView*) override;

#if !defined(OS_MACOSX)
  // content::DelegatedFrameHostClient:
  int DelegatedFrameHostGetGpuMemoryBufferClientId(void) const;
  ui::Layer *DelegatedFrameHostGetLayer(void) const override;
  bool DelegatedFrameHostIsVisible(void) const override;
  SkColor DelegatedFrameHostGetGutterColor(SkColor) const override;
  gfx::Size DelegatedFrameHostDesiredSizeInDIP(void) const override;
  bool DelegatedFrameCanCreateResizeLock(void) const override;
  std::unique_ptr<content::ResizeLock> DelegatedFrameHostCreateResizeLock(
    bool defer_compositor_lock) override;
  void DelegatedFrameHostResizeLockWasReleased(void) override;
  void DelegatedFrameHostSendReclaimCompositorResources(
      int output_surface_id,
      bool is_swap_ack,
      const cc::ReturnedResourceArray& resources) override;
  void SetBeginFrameSource(cc::BeginFrameSource* source) override;
#endif  // !defined(OS_MACOSX)

  bool TransformPointToLocalCoordSpace(
      const gfx::Point& point,
      const cc::SurfaceId& original_surface,
      gfx::Point* transformed_point) override;
  bool TransformPointToCoordSpaceForView(
      const gfx::Point& point,
      RenderWidgetHostViewBase* target_view,
      gfx::Point* transformed_point) override;

  // ui::CompositorDelegate:
  std::unique_ptr<cc::SoftwareOutputDevice> CreateSoftwareOutputDevice(
      ui::Compositor* compositor) override;

  bool InstallTransparency();
  bool IsAutoResizeEnabled() const;

  // NativeWindowObserver:
  void OnWindowResize() override;
  void OnWindowClosed() override;

  void OnBeginFrameTimerTick();
  void SendBeginFrame(base::TimeTicks frame_time,
                      base::TimeDelta vsync_period);

#if defined(OS_MACOSX)
  void CreatePlatformWidget(bool is_guest_view_hack);
  void DestroyPlatformWidget();
#endif

  void CancelWidget();
  void AddGuestHostView(OffScreenRenderWidgetHostView* guest_host);
  void RemoveGuestHostView(OffScreenRenderWidgetHostView* guest_host);
  void AddViewProxy(OffscreenViewProxy* proxy);
  void RemoveViewProxy(OffscreenViewProxy* proxy);
  void ProxyViewDestroyed(OffscreenViewProxy* proxy);

  void RegisterGuestViewFrameSwappedCallback(
      content::RenderWidgetHostViewGuest* guest_host_view);
  void OnGuestViewFrameSwapped(
      content::RenderWidgetHostViewGuest* guest_host_view);

  void OnPaint(const gfx::Rect& damage_rect, const SkBitmap& bitmap);
  void OnPopupPaint(const gfx::Rect& damage_rect, const SkBitmap& bitmap);
  void OnProxyViewPaint(const gfx::Rect& damage_rect);

  bool IsPopupWidget() const {
    return popup_type_ != blink::WebPopupTypeNone;
  }

  void HoldResize();
  void ReleaseResize();
  void WasResized();

  void ProcessKeyboardEvent(
      const content::NativeWebKeyboardEvent& event) override;
  void ProcessMouseEvent(const blink::WebMouseEvent& event,
      const ui::LatencyInfo& latency) override;
  void ProcessMouseWheelEvent(const blink::WebMouseWheelEvent& event,
      const ui::LatencyInfo& latency) override;

  void SetPainting(bool painting);
  bool IsPainting() const;

  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;

  ui::Compositor* GetCompositor() const;
  ui::Layer* GetRootLayer() const;
  content::DelegatedFrameHost* GetDelegatedFrameHost() const;

  void Invalidate();
  void InvalidateBounds(const gfx::Rect&);

  content::RenderWidgetHostImpl* render_widget_host() const
      { return render_widget_host_; }
  NativeWindow* window() const { return native_window_; }
  gfx::Size size() const { return size_; }
  float scale_factor() const { return scale_factor_; }

  void set_popup_host_view(OffScreenRenderWidgetHostView* popup_view) {
    popup_host_view_ = popup_view;
  }

  void set_child_host_view(OffScreenRenderWidgetHostView* child_view) {
    child_host_view_ = child_view;
  }

 private:
  void SetupFrameRate(bool force);
  void ResizeRootLayer();

  cc::FrameSinkId AllocateFrameSinkId(bool is_guest_view_hack);

  // Weak ptrs.
  content::RenderWidgetHostImpl* render_widget_host_;

  OffScreenRenderWidgetHostView* parent_host_view_;
  OffScreenRenderWidgetHostView* popup_host_view_;
  std::unique_ptr<SkBitmap> popup_bitmap_;
  OffScreenRenderWidgetHostView* child_host_view_;
  std::set<OffScreenRenderWidgetHostView*> guest_host_views_;
  std::set<OffscreenViewProxy*> proxy_views_;

  NativeWindow* native_window_;
  OffScreenOutputDevice* software_output_device_;

  const bool transparent_;
  OnPaintCallback callback_;
  OnPaintCallback parent_callback_;

  int frame_rate_;
  int frame_rate_threshold_us_;

  base::Time last_time_;

  float scale_factor_;
  gfx::Vector2dF last_scroll_offset_;
  gfx::Size size_;
  bool painting_;

  bool is_showing_;
  bool is_destroyed_;
  gfx::Rect popup_position_;

  bool hold_resize_;
  bool pending_resize_;

  std::unique_ptr<ui::Layer> root_layer_;
  std::unique_ptr<ui::Compositor> compositor_;
  std::unique_ptr<content::DelegatedFrameHost> delegated_frame_host_;

  std::unique_ptr<AtomCopyFrameGenerator> copy_frame_generator_;
  std::unique_ptr<AtomBeginFrameTimer> begin_frame_timer_;

  // Provides |source_id| for BeginFrameArgs that we create.
  cc::StubBeginFrameSource begin_frame_source_;
  uint64_t begin_frame_number_ = cc::BeginFrameArgs::kStartingFrameNumber;

#if defined(OS_MACOSX)
  CALayer* background_layer_;
  std::unique_ptr<content::BrowserCompositorMac> browser_compositor_;

  // Can not be managed by smart pointer because its header can not be included
  // in the file that has the destructor.
  MacHelper* mac_helper_;

  // Selected text on the renderer.
  std::string selected_text_;
#endif

  base::WeakPtrFactory<OffScreenRenderWidgetHostView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OffScreenRenderWidgetHostView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_
