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
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/base/ime/text_input_client.h"
#include "base/process/kill.h"

#include "cc/output/software_output_device.h"
#include "cc/output/output_surface.h"
#include "cc/output/output_surface_client.h"
#include "cc/scheduler/begin_frame_source.h"

#include <windows.h>
#include "ui/gfx/win/window_impl.h"

namespace atom {

class AtomCompositorHostWin : public gfx::WindowImpl {
 public:
  AtomCompositorHostWin() {
    // Create a hidden 1x1 borderless window.
    set_window_style(WS_POPUP | WS_SYSMENU);
    Init(NULL, gfx::Rect(0, 0, 1, 1));
  }

  ~AtomCompositorHostWin() override {
    DestroyWindow(hwnd());
  }

 private:
  CR_BEGIN_MSG_MAP_EX(CompositorHostWin)
    CR_MSG_WM_PAINT(OnPaint)
  CR_END_MSG_MAP()

  void OnPaint(HDC dc) {
    ValidateRect(hwnd(), NULL);
  }
};

class OffScreenWindow
    : public content::RenderWidgetHostViewBase,
      public content::DelegatedFrameHostClient,
      public cc::BeginFrameObserver,
      public ui::LayerDelegate,
      public ui::LayerOwner {
public:
  OffScreenWindow(content::RenderWidgetHost*);
  ~OffScreenWindow();

//content::RenderWidgetHostView
  bool OnMessageReceived(const IPC::Message&) override;
  void InitAsChild(gfx::NativeView);
  content::RenderWidgetHost* GetRenderWidgetHost(void) const;
  void SetSize(const gfx::Size &);
  void SetBounds(const gfx::Rect &);
  gfx::Vector2dF GetLastScrollOffset(void) const;
  gfx::NativeView GetNativeView(void) const;
  gfx::NativeViewId GetNativeViewId(void) const;
  gfx::NativeViewAccessible GetNativeViewAccessible(void);
  ui::TextInputClient* GetTextInputClient() override;
  void Focus(void);
  bool HasFocus(void) const;
  bool IsSurfaceAvailableForCopy(void) const;
  void Show(void);
  void Hide(void);
  bool IsShowing(void);
  gfx::Rect GetViewBounds(void) const;
  gfx::Size GetVisibleViewportSize() const override;
  void SetInsets(const gfx::Insets&) override;
  bool LockMouse(void);
  void UnlockMouse(void);
  bool GetScreenColorProfile(std::vector<char>*);

//content::RenderWidgetHostViewBase
  void ClearCompositorFrame(void);
  void InitAsPopup(content::RenderWidgetHostView *, const gfx::Rect &);
  void InitAsFullscreen(content::RenderWidgetHostView *);
  void UpdateCursor(const content::WebCursor &);
  void SetIsLoading(bool);
  void TextInputStateChanged(const ViewHostMsg_TextInputState_Params &);
  void ImeCancelComposition(void);
  void RenderProcessGone(base::TerminationStatus,int);
  void Destroy(void);
  void SetTooltipText(const base::string16 &);
  void SelectionBoundsChanged(const ViewHostMsg_SelectionBounds_Params &);
  void CopyFromCompositingSurface(const gfx::Rect &,
    const gfx::Size &,
    const content::ReadbackRequestCallback &,
    const SkColorType);
  void CopyFromCompositingSurfaceToVideoFrame(
    const gfx::Rect &,
    const scoped_refptr<media::VideoFrame> &,
    const base::Callback<void (const gfx::Rect &, bool),
    base::internal::CopyMode::Copyable> &);
  bool CanCopyToVideoFrame(void) const;
  void BeginFrameSubscription(
    std::unique_ptr<content::RenderWidgetHostViewFrameSubscriber>);
  void EndFrameSubscription();
  bool HasAcceleratedSurface(const gfx::Size &);
  void GetScreenInfo(blink::WebScreenInfo *);
  bool GetScreenColorProfile(blink::WebVector<char>*);
  gfx::Rect GetBoundsInRootWindow(void);
  void LockCompositingSurface(void);
  void UnlockCompositingSurface(void);
  void ImeCompositionRangeChanged(
    const gfx::Range &, const std::vector<gfx::Rect>&);

//content::DelegatedFrameHostClient
  int DelegatedFrameHostGetGpuMemoryBufferClientId(void) const;
  ui::Layer *DelegatedFrameHostGetLayer(void) const;
  bool DelegatedFrameHostIsVisible(void) const;
  SkColor DelegatedFrameHostGetGutterColor(SkColor) const;
  gfx::Size DelegatedFrameHostDesiredSizeInDIP(void) const;
  bool DelegatedFrameCanCreateResizeLock(void) const;
  std::unique_ptr<content::ResizeLock> DelegatedFrameHostCreateResizeLock(bool);
  void DelegatedFrameHostResizeLockWasReleased(void);
  void DelegatedFrameHostSendCompositorSwapAck(
    int, const cc::CompositorFrameAck &);
  void DelegatedFrameHostSendReclaimCompositorResources(
    int, const cc::CompositorFrameAck &);
  void DelegatedFrameHostOnLostCompositorResources(void);
  void DelegatedFrameHostUpdateVSyncParameters(
    const base::TimeTicks &, const base::TimeDelta &);

//cc::BeginFrameObserver
  void OnBeginFrame(const cc::BeginFrameArgs &);
  const cc::BeginFrameArgs & LastUsedBeginFrameArgs(void) const;
  void OnBeginFrameSourcePausedChanged(bool);
  void AsValueInto(base::trace_event::TracedValue *) const;

  gfx::Size GetPhysicalBackingSize() const;
  void UpdateScreenInfo(gfx::NativeView view);
  gfx::Size GetRequestedRendererSize() const;
  void OnSwapCompositorFrame(uint32_t, cc::CompositorFrame);
  uint32_t SurfaceIdNamespaceAtPoint(
    cc::SurfaceHittestDelegate* delegate,
    const gfx::Point& point,
    gfx::Point* transformed_point);
  uint32_t GetSurfaceIdNamespace();

  void OnPaintLayer(const ui::PaintContext &);
  void OnDelegatedFrameDamage(const gfx::Rect &);
  void OnDeviceScaleFactorChanged(float);
  base::Closure PrepareForLayerBoundsChange();
  void OnBoundsChanged();

  void OnSetNeedsBeginFrames(bool);
  void SendSwapCompositorFrame(cc::CompositorFrame *);
private:
  content::RenderWidgetHostImpl* host_;
  content::DelegatedFrameHost* delegated_frame_host_;
  gfx::Vector2dF last_scroll_offset_;
  bool focus_;
  gfx::Size size_;
  float scale_;

  cc::BeginFrameSource* begin_frame_source_;
  cc::BeginFrameArgs last_begin_frame_args_;
  bool needs_begin_frames_;

  base::Thread* thread_;
  ui::Compositor* compositor_;
};

class OffScreenOutputSurface : public cc::OutputSurface {
public:
  OffScreenOutputSurface(
    std::unique_ptr<cc::SoftwareOutputDevice>);
  ~OffScreenOutputSurface();

  void SwapBuffers(cc::CompositorFrame *);
  bool BindToClient(cc::OutputSurfaceClient *);
};

class OffScreenOutputDevice : public cc::SoftwareOutputDevice {
public:
  OffScreenOutputDevice();
  ~OffScreenOutputDevice();

  void Resize(const gfx::Size& pixel_size, float scale_factor);
  SkCanvas* BeginPaint(const gfx::Rect& damage_rect);
  void EndPaint();
  void DiscardBackbuffer();
  void EnsureBackbuffer();
  gfx::VSyncProvider* GetVSyncProvider();
};


}  // namespace atom

#endif  // ATOM_BROWSER_OSR_WINDOW_H_
