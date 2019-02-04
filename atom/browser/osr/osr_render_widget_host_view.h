// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_
#define ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "atom/browser/osr/osr_output_device.h"
#include "atom/browser/osr/osr_view_proxy.h"
#include "base/process/kill.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "content/browser/frame_host/render_widget_host_view_guest.h"
#include "content/browser/renderer_host/delegated_frame_host.h"
#include "content/browser/renderer_host/input/mouse_wheel_phase_handler.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/skia/include/core/SkBitmap.h"
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

namespace content {
class CursorManager;
}  // namespace content

namespace atom {

class AtomCopyFrameGenerator;
class AtomBeginFrameTimer;

#if defined(OS_MACOSX)
class MacHelper;
#else
class AtomDelegatedFrameHostClient;
#endif

class OffScreenRenderWidgetHostView : public content::RenderWidgetHostViewBase,
                                      public ui::CompositorDelegate,
                                      public OffscreenViewProxyObserver {
 public:
  OffScreenRenderWidgetHostView(bool transparent,
                                bool painting,
                                int frame_rate,
                                const OnPaintCallback& callback,
                                content::RenderWidgetHost* render_widget_host,
                                OffScreenRenderWidgetHostView* parent_host_view,
                                gfx::Size initial_size);
  ~OffScreenRenderWidgetHostView() override;

  content::BrowserAccessibilityManager* CreateBrowserAccessibilityManager(
      content::BrowserAccessibilityDelegate*,
      bool) override;

  // content::RenderWidgetHostView:
  void InitAsChild(gfx::NativeView) override;
  void SetSize(const gfx::Size&) override;
  void SetBounds(const gfx::Rect&) override;
  gfx::NativeView GetNativeView(void) const override;
  gfx::NativeViewAccessible GetNativeViewAccessible(void) override;
  ui::TextInputClient* GetTextInputClient() override;
  void Focus(void) override;
  bool HasFocus(void) const override;
  uint32_t GetCaptureSequenceNumber() const override;
  bool IsSurfaceAvailableForCopy(void) const override;
  void Show(void) override;
  void Hide(void) override;
  bool IsShowing(void) override;
  void EnsureSurfaceSynchronizedForWebTest() override;
  gfx::Rect GetViewBounds(void) const override;
  gfx::Size GetVisibleViewportSize() const override;
  void SetInsets(const gfx::Insets&) override;
  void SetBackgroundColor(SkColor color) override;
  base::Optional<SkColor> GetBackgroundColor() const override;
  void UpdateBackgroundColor() override;
  bool LockMouse(void) override;
  void UnlockMouse(void) override;
  void TakeFallbackContentFrom(content::RenderWidgetHostView* view) override;
  void SetNeedsBeginFrames(bool needs_begin_frames) override;
  void SetWantsAnimateOnlyBeginFrames() override;
#if defined(OS_MACOSX)
  void SetActive(bool active) override;
  void ShowDefinitionForSelection() override;
  void SpeakSelection() override;
  bool UpdateNSViewAndDisplay();
#endif  // defined(OS_MACOSX)

  // content::RenderWidgetHostViewBase:
  void DidCreateNewRendererCompositorFrameSink(
      viz::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink)
      override;
  void SubmitCompositorFrame(
      const viz::LocalSurfaceId& local_surface_id,
      viz::CompositorFrame frame,
      base::Optional<viz::HitTestRegionList> hit_test_region_list) override;

  void ClearCompositorFrame(void) override;
  void ResetFallbackToFirstNavigationSurface() override;
  void InitAsPopup(content::RenderWidgetHostView* rwhv,
                   const gfx::Rect& rect) override;
  void InitAsFullscreen(content::RenderWidgetHostView*) override;
  void UpdateCursor(const content::WebCursor&) override;
  void SetIsLoading(bool is_loading) override;
  void TextInputStateChanged(const content::TextInputState& params) override;
  void ImeCancelComposition(void) override;
  void RenderProcessGone(base::TerminationStatus, int) override;
  void Destroy(void) override;
  void SetTooltipText(const base::string16&) override;
  content::CursorManager* GetCursorManager() override;
  void CopyFromSurface(
      const gfx::Rect& src_rect,
      const gfx::Size& output_size,
      base::OnceCallback<void(const SkBitmap&)> callback) override;
  void GetScreenInfo(content::ScreenInfo* results) const override;
  void InitAsGuest(content::RenderWidgetHostView*,
                   content::RenderWidgetHostViewGuest*) override;
  void TransformPointToRootSurface(gfx::PointF* point) override;
  gfx::Rect GetBoundsInRootWindow(void) override;
  viz::SurfaceId GetCurrentSurfaceId() const override;
  void ImeCompositionRangeChanged(const gfx::Range&,
                                  const std::vector<gfx::Rect>&) override;
  gfx::Size GetCompositorViewportPixelSize() const override;

#if defined(OS_MACOSX)
  viz::ScopedSurfaceIdAllocator DidUpdateVisualProperties(
      const cc::RenderFrameMetadata& metadata) override;
#endif

  content::RenderWidgetHostViewBase* CreateViewForWidget(
      content::RenderWidgetHost*,
      content::RenderWidgetHost*,
      content::WebContentsView*) override;

  const viz::LocalSurfaceIdAllocation& GetLocalSurfaceIdAllocation()
      const override;
  const viz::FrameSinkId& GetFrameSinkId() const override;

  void DidNavigate() override;

  bool TransformPointToLocalCoordSpaceLegacy(
      const gfx::PointF& point,
      const viz::SurfaceId& original_surface,
      gfx::PointF* transformed_point) override;
  bool TransformPointToCoordSpaceForView(
      const gfx::PointF& point,
      RenderWidgetHostViewBase* target_view,
      gfx::PointF* transformed_point,
      viz::EventSource source = viz::EventSource::ANY) override;

  // ui::CompositorDelegate:
  std::unique_ptr<viz::SoftwareOutputDevice> CreateSoftwareOutputDevice(
      ui::Compositor* compositor) override;

  bool InstallTransparency();

  void OnBeginFrameTimerTick();
  void SendBeginFrame(base::TimeTicks frame_time, base::TimeDelta vsync_period);

#if defined(OS_MACOSX)
  void CreatePlatformWidget(bool is_guest_view_hack);
  void DestroyPlatformWidget();
  SkColor last_frame_root_background_color() const {
    return last_frame_root_background_color_;
  }
#endif

  void CancelWidget();
  void AddGuestHostView(OffScreenRenderWidgetHostView* guest_host);
  void RemoveGuestHostView(OffScreenRenderWidgetHostView* guest_host);
  void AddViewProxy(OffscreenViewProxy* proxy);
  void RemoveViewProxy(OffscreenViewProxy* proxy);
  void ProxyViewDestroyed(OffscreenViewProxy* proxy) override;

  void OnPaint(const gfx::Rect& damage_rect, const SkBitmap& bitmap);
  void OnPopupPaint(const gfx::Rect& damage_rect, const SkBitmap& bitmap);
  void OnProxyViewPaint(const gfx::Rect& damage_rect) override;

  bool IsPopupWidget() const {
    return widget_type_ == content::WidgetType::kPopup;
  }

  void HoldResize();
  void ReleaseResize();
  void SynchronizeVisualProperties();

  void SendMouseEvent(const blink::WebMouseEvent& event);
  void SendMouseWheelEvent(const blink::WebMouseWheelEvent& event);

  void SetPainting(bool painting);
  bool IsPainting() const;

  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;

  ui::Compositor* GetCompositor() const;
  ui::Layer* GetRootLayer() const;

#if defined(OS_MACOSX)
  content::BrowserCompositorMac* browser_compositor() const {
    return browser_compositor_.get();
  }
#endif

  content::DelegatedFrameHost* GetDelegatedFrameHost() const;

  void Invalidate();
  void InvalidateBounds(const gfx::Rect&);

  content::RenderWidgetHostImpl* render_widget_host() const {
    return render_widget_host_;
  }

  gfx::Size size() const { return size_; }

  void set_popup_host_view(OffScreenRenderWidgetHostView* popup_view) {
    popup_host_view_ = popup_view;
  }

  void set_child_host_view(OffScreenRenderWidgetHostView* child_view) {
    child_host_view_ = child_view;
  }

 private:
#if defined(OS_MACOSX)
  display::Display GetDisplay();
  void OnDidUpdateVisualPropertiesComplete(
      const cc::RenderFrameMetadata& metadata);
#endif

  void SetupFrameRate(bool force);
  void ResizeRootLayer(bool force);

  viz::FrameSinkId AllocateFrameSinkId(bool is_guest_view_hack);

  // Applies background color without notifying the RenderWidget about
  // opaqueness changes.
  void UpdateBackgroundColorFromRenderer(SkColor color);

  // Weak ptrs.
  content::RenderWidgetHostImpl* render_widget_host_;

  OffScreenRenderWidgetHostView* parent_host_view_ = nullptr;
  OffScreenRenderWidgetHostView* popup_host_view_ = nullptr;
  std::unique_ptr<SkBitmap> popup_bitmap_;
  OffScreenRenderWidgetHostView* child_host_view_ = nullptr;
  std::set<OffScreenRenderWidgetHostView*> guest_host_views_;
  std::set<OffscreenViewProxy*> proxy_views_;

  OffScreenOutputDevice* software_output_device_ = nullptr;

  const bool transparent_;
  OnPaintCallback callback_;
  OnPaintCallback parent_callback_;

  int frame_rate_ = 0;
  int frame_rate_threshold_us_ = 0;

  base::Time last_time_ = base::Time::Now();

  gfx::Vector2dF last_scroll_offset_;
  gfx::Size size_;
  bool painting_;

  bool is_showing_ = false;
  bool is_destroyed_ = false;
  gfx::Rect popup_position_;

  bool hold_resize_ = false;
  bool pending_resize_ = false;

  bool paint_callback_running_ = false;

  viz::LocalSurfaceIdAllocation local_surface_id_allocation_;
  viz::ParentLocalSurfaceIdAllocator local_surface_id_allocator_;

  std::unique_ptr<ui::Layer> root_layer_;
  std::unique_ptr<ui::Compositor> compositor_;
  std::unique_ptr<content::DelegatedFrameHost> delegated_frame_host_;

  std::unique_ptr<content::CursorManager> cursor_manager_;

  std::unique_ptr<AtomCopyFrameGenerator> copy_frame_generator_;
  std::unique_ptr<AtomBeginFrameTimer> begin_frame_timer_;

  // Provides |source_id| for BeginFrameArgs that we create.
  viz::StubBeginFrameSource begin_frame_source_;
  uint64_t begin_frame_number_ = viz::BeginFrameArgs::kStartingFrameNumber;

#if defined(OS_MACOSX)
  std::unique_ptr<content::BrowserCompositorMac> browser_compositor_;

  SkColor last_frame_root_background_color_;

  // Can not be managed by smart pointer because its header can not be included
  // in the file that has the destructor.
  MacHelper* mac_helper_;

  // Selected text on the renderer.
  std::string selected_text_;
#else
  std::unique_ptr<AtomDelegatedFrameHostClient> delegated_frame_host_client_;
#endif

  content::MouseWheelPhaseHandler mouse_wheel_phase_handler_;

  viz::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink_ =
      nullptr;

  // Latest capture sequence number which is incremented when the caller
  // requests surfaces be synchronized via
  // EnsureSurfaceSynchronizedForWebTest().
  uint32_t latest_capture_sequence_number_ = 0u;

  SkColor background_color_ = SkColor();

  base::WeakPtrFactory<OffScreenRenderWidgetHostView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OffScreenRenderWidgetHostView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_
