// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_
#define ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_

#include <string>
#include <vector>

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "atom/browser/native_window.h"
#include "atom/browser/osr/osr_output_device.h"
#include "base/process/kill.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "cc/scheduler/begin_frame_source.h"
#include "cc/output/compositor_frame.h"
#include "content/browser/renderer_host/delegated_frame_host.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/renderer_host/resize_lock.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/gfx/geometry/point.h"
#include "third_party/WebKit/public/platform/WebVector.h"

#if defined(OS_WIN)
#include "ui/gfx/win/window_impl.h"
#endif

#if defined(OS_MACOSX)
#include "content/browser/renderer_host/browser_compositor_view_mac.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"
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

class OffScreenRenderWidgetHostView
    : public content::RenderWidgetHostViewBase,
#if defined(OS_MACOSX)
      public ui::AcceleratedWidgetMacNSView,
#endif
      public ui::CompositorDelegate,
      public content::DelegatedFrameHostClient {
 public:
  OffScreenRenderWidgetHostView(bool transparent,
                                const OnPaintCallback& callback,
                                content::RenderWidgetHost* render_widget_host,
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

  // content::RenderWidgetHostViewBase:
  void OnSwapCompositorFrame(uint32_t, std::unique_ptr<cc::CompositorFrame>)
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
  void CopyFromCompositingSurface(const gfx::Rect &,
    const gfx::Size &,
    const content::ReadbackRequestCallback &,
    const SkColorType) override;
  void CopyFromCompositingSurfaceToVideoFrame(
    const gfx::Rect &,
    const scoped_refptr<media::VideoFrame> &,
    const base::Callback<void(const gfx::Rect &, bool),
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
  void DelegatedFrameHostSendCompositorSwapAck(
    int, const cc::CompositorFrameAck &) override;
  void DelegatedFrameHostSendReclaimCompositorResources(
    int, const cc::CompositorFrameAck &) override;
  void DelegatedFrameHostOnLostCompositorResources(void) override;
  void DelegatedFrameHostUpdateVSyncParameters(
    const base::TimeTicks &, const base::TimeDelta &) override;
  void SetBeginFrameSource(cc::BeginFrameSource* source) override;

  // ui::CompositorDelegate:
  std::unique_ptr<cc::SoftwareOutputDevice> CreateSoftwareOutputDevice(
      ui::Compositor* compositor) override;

  bool InstallTransparency();
  bool IsAutoResizeEnabled() const;
  void OnSetNeedsBeginFrames(bool enabled);

#if defined(OS_MACOSX)
  // ui::AcceleratedWidgetMacNSView:
  NSView* AcceleratedWidgetGetNSView() const override;
  void AcceleratedWidgetGetVSyncParameters(
      base::TimeTicks* timebase, base::TimeDelta* interval) const override;
  void AcceleratedWidgetSwapCompleted() override;
#endif  // defined(OS_MACOSX)

  void OnBeginFrameTimerTick();
  void SendBeginFrame(base::TimeTicks frame_time,
                      base::TimeDelta vsync_period);

#if defined(OS_MACOSX)
  void CreatePlatformWidget();
  void DestroyPlatformWidget();
#endif

  void OnPaint(const gfx::Rect& damage_rect, const SkBitmap& bitmap);

  void SetPainting(bool painting);
  bool IsPainting() const;

  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;

  ui::Compositor* compositor() const { return compositor_.get(); }
  content::RenderWidgetHostImpl* render_widget_host() const
      { return render_widget_host_; }

 private:
  void SetupFrameRate(bool force);
  void ResizeRootLayer();

  // Weak ptrs.
  content::RenderWidgetHostImpl* render_widget_host_;
  NativeWindow* native_window_;
  OffScreenOutputDevice* software_output_device_;

  const bool transparent_;
  OnPaintCallback callback_;

  int frame_rate_;
  int frame_rate_threshold_ms_;

  base::Time last_time_;

  float scale_factor_;
  bool is_showing_;
  gfx::Vector2dF last_scroll_offset_;
  gfx::Size size_;
  bool painting_;

  std::unique_ptr<ui::Layer> root_layer_;
  std::unique_ptr<ui::Compositor> compositor_;
  std::unique_ptr<content::DelegatedFrameHost> delegated_frame_host_;

  std::unique_ptr<AtomCopyFrameGenerator> copy_frame_generator_;
  std::unique_ptr<AtomBeginFrameTimer> begin_frame_timer_;

#if defined(OS_MACOSX)
  NSWindow* window_;
  CALayer* background_layer_;
  std::unique_ptr<content::BrowserCompositorMac> browser_compositor_;

  // Selected text on the renderer.
  std::string selected_text_;
#endif

  base::WeakPtrFactory<OffScreenRenderWidgetHostView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OffScreenRenderWidgetHostView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_OSR_OSR_RENDER_WIDGET_HOST_VIEW_H_
