// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_WINDOW_H_
#define ATOM_BROWSER_OSR_WINDOW_H_

#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/renderer_host/delegated_frame_host.h"
#include "content/browser/renderer_host/resize_lock.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "cc/scheduler/begin_frame_source.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "cc/output/compositor_frame.h"
#include "ui/gfx/geometry/point.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/base/ime/text_input_client.h"
#include "base/process/kill.h"

#include "cc/output/software_output_device.h"
#include "cc/output/output_surface.h"
#include "cc/output/output_surface_client.h"
#include "cc/scheduler/begin_frame_source.h"

#if defined(OS_WIN)
#include <windows.h>
#include "ui/gfx/win/window_impl.h"
#endif

#if defined(OS_MACOSX)
#include "content/browser/renderer_host/browser_compositor_view_mac.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"
#endif

#include "atom/browser/osr_output_device.h"

#if defined(OS_MACOSX)
#ifdef __OBJC__
@class CALayer;
@class NSWindow;
@class NSTextInputContext;
#else
class CALayer;
class NSWindow;
class NSTextInputContext;
#endif
#endif

namespace atom {

class CefCopyFrameGenerator;
class CefBeginFrameTimer;

class OffScreenWindow
  : public content::RenderWidgetHostViewBase,
  #if defined(OS_MACOSX)
    public ui::AcceleratedWidgetMacNSView,
  #endif
    // public ui::CompositorDelegate,
    public content::DelegatedFrameHostClient {
public:
  typedef base::Callback<void(const gfx::Rect&,int,int,void*)>
      OnPaintCallback;

  OffScreenWindow(content::RenderWidgetHost*);
  ~OffScreenWindow();

  void CreatePlatformWidget();

  // content::RenderWidgetHostView
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
  bool LockMouse(void) override;
  void UnlockMouse(void) override;
  bool GetScreenColorProfile(std::vector<char>*) override;

#if defined(OS_MACOSX)
  ui::AcceleratedWidgetMac* GetAcceleratedWidgetMac() const override;
  void SetActive(bool active) override;
  void ShowDefinitionForSelection() override;
  bool SupportsSpeech() const override;
  void SpeakSelection() override;
  bool IsSpeaking() const override;
  void StopSpeaking() override;
#endif  // defined(OS_MACOSX)

  // content::RenderWidgetHostViewBase
  void OnSwapCompositorFrame(uint32_t, std::unique_ptr<cc::CompositorFrame>) override;
  void ClearCompositorFrame(void) override;
  void InitAsPopup(content::RenderWidgetHostView *, const gfx::Rect &) override;
  void InitAsFullscreen(content::RenderWidgetHostView *) override;
  void UpdateCursor(const content::WebCursor &) override;
  void SetIsLoading(bool) override;
  void TextInputStateChanged(const content::TextInputState& params) override;
  void ImeCancelComposition(void) override;
  void RenderProcessGone(base::TerminationStatus,int) override;
  void Destroy(void) override;
  void SetTooltipText(const base::string16 &) override;

#if defined(OS_MACOSX)
  void SelectionChanged(const base::string16& text,
                        size_t offset,
                        const gfx::Range& range) override;
#endif

  void SelectionBoundsChanged(const ViewHostMsg_SelectionBounds_Params &) override;
  void CopyFromCompositingSurface(const gfx::Rect &,
    const gfx::Size &,
    const content::ReadbackRequestCallback &,
    const SkColorType) override;
  void CopyFromCompositingSurfaceToVideoFrame(
    const gfx::Rect &,
    const scoped_refptr<media::VideoFrame> &,
    const base::Callback<void (const gfx::Rect &, bool),
    base::internal::CopyMode::Copyable> &) override;
  bool CanCopyToVideoFrame(void) const override;
  void BeginFrameSubscription(
    std::unique_ptr<content::RenderWidgetHostViewFrameSubscriber>) override;
  void EndFrameSubscription() override;
  bool HasAcceleratedSurface(const gfx::Size &) override;
  void GetScreenInfo(blink::WebScreenInfo *) override;
  bool GetScreenColorProfile(blink::WebVector<char>*);
  gfx::Rect GetBoundsInRootWindow(void) override;
  void LockCompositingSurface(void) override;
  void UnlockCompositingSurface(void) override;
  void ImeCompositionRangeChanged(
    const gfx::Range &, const std::vector<gfx::Rect>&) override;
  gfx::Size GetPhysicalBackingSize() const override;
  gfx::Size GetRequestedRendererSize() const override;

  // content::DelegatedFrameHostClient
  int DelegatedFrameHostGetGpuMemoryBufferClientId(void) const;
  ui::Layer *DelegatedFrameHostGetLayer(void) const override;
  bool DelegatedFrameHostIsVisible(void) const override;
  SkColor DelegatedFrameHostGetGutterColor(SkColor) const override;
  gfx::Size DelegatedFrameHostDesiredSizeInDIP(void) const override;
  bool DelegatedFrameCanCreateResizeLock(void) const override;
  std::unique_ptr<content::ResizeLock> DelegatedFrameHostCreateResizeLock(bool) override;
  void DelegatedFrameHostResizeLockWasReleased(void) override;
  void DelegatedFrameHostSendCompositorSwapAck(
    int, const cc::CompositorFrameAck &) override;
  void DelegatedFrameHostSendReclaimCompositorResources(
    int, const cc::CompositorFrameAck &) override;
  void DelegatedFrameHostOnLostCompositorResources(void) override;
  void DelegatedFrameHostUpdateVSyncParameters(
    const base::TimeTicks &, const base::TimeDelta &) override;
  void SetBeginFrameSource(cc::BeginFrameSource* source) override;

  bool IsAutoResizeEnabled() const;

  // std::unique_ptr<cc::SoftwareOutputDevice> CreateSoftwareOutputDevice(
  //     ui::Compositor* compositor) override;
  void OnSetNeedsBeginFrames(bool enabled);

#if defined(OS_MACOSX)
  // AcceleratedWidgetMacNSView implementation.
  NSView* AcceleratedWidgetGetNSView() const override;
  void AcceleratedWidgetGetVSyncParameters(
      base::TimeTicks* timebase, base::TimeDelta* interval) const override;
  void AcceleratedWidgetSwapCompleted() override;
#endif  // defined(OS_MACOSX)

  ui::Compositor* compositor() const { return compositor_.get(); }
  content::RenderWidgetHostImpl* render_widget_host() const
      { return render_widget_host_; }

  void OnBeginFrameTimerTick();
  void SendBeginFrame(base::TimeTicks frame_time,
                      base::TimeDelta vsync_period);

  std::unique_ptr<const OnPaintCallback> paintCallback;
  void SetPaintCallback(const OnPaintCallback*);

private:
  void SetFrameRate();
  void ResizeRootLayer();

  content::RenderWidgetHostImpl* render_widget_host_;

  std::unique_ptr<CefCopyFrameGenerator> copy_frame_generator_;
  std::unique_ptr<CefBeginFrameTimer> begin_frame_timer_;

  OffScreenOutputDevice* software_output_device_;

  int frame_rate_threshold_ms_;

  base::Time last_time_;

  float scale_factor_;
  bool is_showing_;
  gfx::Vector2dF last_scroll_offset_;
  gfx::Size size_;

  std::unique_ptr<content::DelegatedFrameHost> delegated_frame_host_;
  std::unique_ptr<ui::Compositor> compositor_;
  gfx::AcceleratedWidget compositor_widget_;
  std::unique_ptr<ui::Layer> root_layer_;

#if defined(OS_WIN)
  std::unique_ptr<gfx::WindowImpl> window_;
#elif defined(OS_MACOSX)
  NSWindow* window_;
  CALayer* background_layer_;
  std::unique_ptr<content::BrowserCompositorMac> browser_compositor_;
#elif defined(USE_X11)
  CefWindowX11* window_;
  std::unique_ptr<ui::XScopedCursor> invisible_cursor_;
#endif

#if defined(OS_MACOSX)
  NSTextInputContext* text_input_context_osr_mac_;

  // Selected text on the renderer.
  std::string selected_text_;

  // The current composition character range and its bounds.
  gfx::Range composition_range_;
  std::vector<gfx::Rect> composition_bounds_;

  // The current caret bounds.
  gfx::Rect caret_rect_;

  // The current first selection bounds.
  gfx::Rect first_selection_rect_;
#endif

  base::WeakPtrFactory<OffScreenWindow> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(OffScreenWindow);
};

}  // namespace atom

#endif  // ATOM_BROWSER_OSR_WINDOW_H_
